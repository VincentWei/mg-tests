///////////////////////////////////////////////////////////////////////////////
//
//                        IMPORTANT LEGAL NOTICE
//
// The following open source license statement does not apply to any
// entity in the Exception List published by FMSoft.
//
// For more information, please visit:
//
// https://www.fmsoft.cn/exception-list
//
//////////////////////////////////////////////////////////////////////////////
/*
** intel_i915_driver.c: A sample DRM driver for MiniGUI 4.0.7 or later.
**
** This driver is derived from Mesa.
**
** Copyright (C) 2020 FMSoft (http://www.fmsoft.cn).
**
** Copyright notice of Mesa:
**
** Copyright 2003 VMware, Inc.
** All Rights Reserved.
**
** Permission is hereby granted, free of charge, to any person obtaining a
** copy of this software and associated documentation files (the
** "Software"), to deal in the Software without restriction, including
** without limitation the rights to use, copy, modify, merge, publish,
** distribute, sub license, and/or sell copies of the Software, and to
** permit persons to whom the Software is furnished to do so, subject to
** the following conditions:
**
** The above copyright notice and this permission notice (including the
** next paragraph) shall be included in all copies or substantial portions
** of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
** OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
** IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
** ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
** TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
** SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#undef _DEBUG

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/exstubs.h>

#ifdef _MGGAL_DRM

#ifdef HAVE_DRM_INTEL

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <libudev.h>
#include <drm.h>
#include <drm_fourcc.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <i915_drm.h>
#include <libdrm/intel_bufmgr.h>

#include "intel_reg.h"
#include "intel_context.h"
#include "intel_batchbuffer.h"
#include "intel_chipset.h"

static void intel_batchbuffer_reset(struct _DrmDriver *driver)
{
    drm_intel_gem_bo_clear_relocs(driver->batch.bo, 0);
    driver->batch.reserved_space = BATCH_RESERVED;
    driver->batch.used = 0;
}

static void intel_batchbuffer_init(struct _DrmDriver *driver)
{
    driver->batch.bo = drm_intel_bo_alloc(driver->manager, "batchbuffer",
            driver->maxBatchSize, BATCH_SIZE);
    drm_intel_bufmgr_gem_enable_reuse(driver->manager);

    intel_batchbuffer_reset(driver);
    driver->batch.cpu_map = malloc(driver->maxBatchSize);
    driver->batch.map = driver->batch.cpu_map;
}

static void intel_batchbuffer_free(struct _DrmDriver *driver)
{
    free(driver->batch.cpu_map);
    drm_intel_bo_unreference(driver->batch.bo);
}

static int intel_do_flush_locked(struct _DrmDriver *driver, unsigned int flags)
{
    struct intel_batchbuffer *batch = &driver->batch;
    int ret = 0;

    ret = drm_intel_bo_subdata(batch->bo, 0, 4 * batch->used, batch->map);

    if (ret == 0) {
        ret = drm_intel_bo_mrb_exec(batch->bo, 4 * batch->used, NULL, 0, 0,
                flags);
    }
    else {
        _DBG_PRINTF("drm_intel_bo_subdata failed (%p): %s\n",
                batch->bo, strerror(-ret));
    }

    return ret;
}

static int intel_batchbuffer_flush(struct _DrmDriver *driver, unsigned int flags)
{
    int ret;

    if (driver->batch.used == 0)
        return 0;

    driver->batch.reserved_space = 0;

    /* Mark the end of the buffer. */
    intel_batchbuffer_emit_dword(driver, MI_BATCH_BUFFER_END);
    if (driver->batch.used & 1) {
        /* Round batchbuffer usage to 2 DWORDs. */
        intel_batchbuffer_emit_dword(driver, MI_NOOP);
    }

    ret = intel_do_flush_locked(driver, flags);

    /* Reset the buffer */
    intel_batchbuffer_reset(driver);
    return ret;
}

static bool intel_batchbuffer_emit_reloc_fenced(struct _DrmDriver *driver,
        drm_intel_bo *buffer,
        uint32_t read_domains,
        uint32_t write_domain,
        uint32_t delta)
{
    int ret;

    ret = drm_intel_bo_emit_reloc_fence(driver->batch.bo, 4*driver->batch.used,
            buffer, delta,
            read_domains, write_domain);
    assert(ret == 0);
    (void)ret;

    /*
     * Using the old buffer offset, write in what the right data would
     * be, in case the buffer doesn't move and we can short-circuit the
     * relocation processing in the kernel
     */
    intel_batchbuffer_emit_dword(driver, buffer->offset + delta);

    return true;
}

/* Emit a pipelined flush to either flush render and texture cache for
 * reading from a FBO-drawn texture, or flush so that frontbuffer
 * render appears on the screen in DRI1.
 *
 * This is also used for the always_flush_cache driconf debug option.
 */
static void intel_batchbuffer_emit_mi_flush(struct _DrmDriver *driver)
{
    intel_batchbuffer_begin(driver, 1);
    intel_batchbuffer_emit_dword(driver, MI_FLUSH);
    intel_batchbuffer_advance(driver);
    intel_batchbuffer_flush(driver,
            (driver->gen > 4) ? I915_EXEC_BLT : I915_EXEC_RENDER);
}

    static const char *
get_udev_property(struct udev_device *device, const char *name)
{
    struct udev_list_entry *entry;

    udev_list_entry_foreach (entry,
            udev_device_get_properties_list_entry (device))
    {
        if (strcmp (udev_list_entry_get_name (entry), name) == 0)
            return udev_list_entry_get_value (entry);
    }

    return NULL;
}

static inline int pci_id_to_gen (int chip_id)
{
    int gen;

    if (intel_get_genx(chip_id, &gen))
        ;
    else if (IS_GEN8(chip_id))
        gen = 8;
    else if (IS_GEN7(chip_id))
        gen = 7;
    else if (IS_GEN6(chip_id))
        gen = 6;
    else if (IS_GEN5(chip_id))
        gen = 5;
    else if (IS_GEN4(chip_id))
        gen = 4;
    else if (IS_9XX(chip_id))
        gen = 3;
    else {
        assert(IS_GEN2(chip_id));
        gen = 2;
    }

    return gen;
}

#if 0   /* deprecated code */
/* This function overrides the INTEL_DEVID_OVERRIDE environment variable
   to avoid executing ioctl DRM_IOCTL_I915_GETPARAM.
   Because when a client of MiniGUI-Processes initializes the buffer manager,
   the function get_pci_device_id will fail, while the function executing
   ioctl DRM_IOCTL_I915_GETPARAM to get the PCI device id.
 */
static inline void override_devid (int pci_id, int gen)
{
    static const struct {
        const char *name;
        int pci_id;
    } name_map[] = {
        { "brw", PCI_CHIP_I965_GM },
        { "g4x", PCI_CHIP_GM45_GM },
        { "ilk", PCI_CHIP_ILD_G },
        { "snb", PCI_CHIP_SANDYBRIDGE_M_GT2_PLUS },
        { "ivb", PCI_CHIP_IVYBRIDGE_S_GT2 },    // gen 7
        { "hsw", PCI_CHIP_HASWELL_CRW_E_GT3 },
        { "byt", PCI_CHIP_VALLEYVIEW_3 },
        { "bdw", 0x1620 | BDW_ULX },
        { "skl", PCI_CHIP_SKYLAKE_DT_GT2 },
        { "kbl", PCI_CHIP_KABYLAKE_DT_GT2 },
    };
    unsigned int i;

    for (i = 0; i < TABLESIZE(name_map); i++) {
        if (pci_id_to_gen (name_map[i].pci_id) == gen) {
            setenv("INTEL_DEVID_OVERRIDE", name_map[i].name, 1);
            _WRN_PRINTF ("override devid: %s\n", name_map[i].name);
            break;
        }
    }
}
#endif  /* deprecated code */

static int get_intel_chip_id (DrmDriver *driver, int fd)
{
    struct stat st;
    struct udev *udev = NULL;
    struct udev_device *device = NULL;

    if (fstat (fd, &st) < 0 || ! S_ISCHR (st.st_mode)) {
        _DBG_PRINTF ("bad file descriptor (%d): %m\n", fd);
        return -1;
    }

    udev = udev_new ();
    device = udev_device_new_from_devnum (udev, 'c', st.st_rdev);
    if (device != NULL) {
        struct udev_device *parent;
        const char *pci_id;
        uint32_t vendor_id;
        uint32_t chip_id;

        parent = udev_device_get_parent (device);
        pci_id = get_udev_property (parent, "PCI_ID");
        if (pci_id == NULL ||
                sscanf (pci_id, "%x:%x", &vendor_id, &chip_id) != 2) {
            _DBG_PRINTF ("bad udev property %s.\n", pci_id);
            goto failed;
        }

        if (vendor_id != 0x8086) {
            _DBG_PRINTF ("not an Intel GPU (%X:%X).\n", vendor_id, chip_id);
            goto failed;
        }

        driver->chip_id = chip_id;
        driver->gen = pci_id_to_gen (chip_id);

        _DBG_PRINTF("chip id: %u, generation: %d\n", chip_id, driver->gen);
    }

    udev_device_unref (device);
    udev_unref (udev);
    return 0;

failed:
    if (device)
        udev_device_unref (device);
    if (udev)
        udev_unref (udev);

    return -1;
}

static DrmDriver* i915_create_driver (int device_fd)
{
    DrmDriver *driver;

    driver = calloc (1, sizeof (DrmDriver));
    driver->device_fd = device_fd;

    if (get_intel_chip_id (driver, device_fd)) {
        _ERR_PRINTF ("DRM>i915: failed to get genenration.\n");
        free (driver);
        return NULL;
    }

    driver->manager = drm_intel_bufmgr_gem_init (driver->device_fd, BATCH_SZ);
    if (driver->manager == NULL) {
        _ERR_PRINTF ("DRM>i915: failed to initialize buffer manager\n");
        free (driver);
        return NULL;
    }

    drm_intel_bufmgr_gem_enable_fenced_relocs(driver->manager);

    driver->nr_buffers = 0;

    driver->maxBatchSize = BATCH_SIZE;
    intel_batchbuffer_init(driver);

    return driver;
}

static void i915_destroy_driver (DrmDriver *driver)
{
    if (driver->nr_buffers) {
        _WRN_PRINTF ("There is still %d buffers left\n", driver->nr_buffers);
    }

    intel_batchbuffer_free(driver);
    drm_intel_bufmgr_destroy (driver->manager);
    free (driver);
}

static void i915_flush (DrmDriver *driver, DrmSurfaceBuffer *buffer,
        const GAL_Rect *dirty_rect)
{
    (void)buffer;
    (void)dirty_rect;
    intel_batchbuffer_emit_mi_flush(driver);
}

typedef struct _my_surface_buffer {
    DrmSurfaceBuffer base;
    drm_intel_bo *bo;
} my_surface_buffer;

#define ROUND_TO_MULTIPLE(n, m) (((n) + (((m) - 1))) & ~((m) - 1))
#define IS_SURFACE_FOR_SCANOUT(flags) \
    ((flags & DRM_SURBUF_TYPE_MASK) == DRM_SURBUF_TYPE_SCANOUT)

static int drm_format_to_bpp(uint32_t drm_format,
        int* bpp, int* cpp)
{
    switch (drm_format) {
        case DRM_FORMAT_RGB332:
        case DRM_FORMAT_BGR233:
            *bpp = 8;
            *cpp = 1;
            break;

        case DRM_FORMAT_XRGB4444:
        case DRM_FORMAT_XBGR4444:
        case DRM_FORMAT_RGBX4444:
        case DRM_FORMAT_BGRX4444:
        case DRM_FORMAT_ARGB4444:
        case DRM_FORMAT_ABGR4444:
        case DRM_FORMAT_RGBA4444:
        case DRM_FORMAT_BGRA4444:
        case DRM_FORMAT_XRGB1555:
        case DRM_FORMAT_XBGR1555:
        case DRM_FORMAT_RGBX5551:
        case DRM_FORMAT_BGRX5551:
        case DRM_FORMAT_ARGB1555:
        case DRM_FORMAT_ABGR1555:
        case DRM_FORMAT_RGBA5551:
        case DRM_FORMAT_BGRA5551:
        case DRM_FORMAT_RGB565:
        case DRM_FORMAT_BGR565:
            *bpp = 16;
            *cpp = 2;
            break;

            /* 24 bpp is not supported by i915
               case DRM_FORMAT_RGB888:
               case DRM_FORMAT_BGR888:
               bpp = 24;
               cpp = 3;
               break;
             */

        case DRM_FORMAT_XRGB8888:
        case DRM_FORMAT_XBGR8888:
        case DRM_FORMAT_RGBX8888:
        case DRM_FORMAT_BGRX8888:
        case DRM_FORMAT_ARGB8888:
        case DRM_FORMAT_ABGR8888:
        case DRM_FORMAT_RGBA8888:
        case DRM_FORMAT_BGRA8888:
            *bpp = 32;
            *cpp = 4;
            break;

        default:
            return 0;
            break;
    }

    return 1;
}

static my_surface_buffer* i915_create_buffer_helper (DrmDriver *driver,
        drm_intel_bo *bo)
{
    my_surface_buffer *buffer;

    buffer = calloc (1, sizeof (my_surface_buffer));
    if (buffer == NULL) {
        _ERR_PRINTF ("DRM>i915: could not allocate surface buffer\n");
        return NULL;
    }

    buffer->base.handle = bo->handle;
    buffer->base.size = bo->size;
    buffer->bo = bo;

    driver->nr_buffers++;

    _DBG_PRINTF("Buffer object (%u) created: size (%lu)\n",
            buffer->base.handle, buffer->base.size);
    return buffer;
}

static DrmSurfaceBuffer* i915_create_buffer (DrmDriver *driver,
        uint32_t drm_format, uint32_t hdr_size,
        uint32_t width, uint32_t height, uint32_t flags)
{
    drm_intel_bo *bo;
    my_surface_buffer *buffer;
    int bpp, cpp;
    uint32_t pitch, nr_hdr_lines = 0;

    if (drm_format_to_bpp(drm_format, &bpp, &cpp) == 0) {
        _ERR_PRINTF ("DRM>i915: not supported format: %d\n", drm_format);
        return NULL;
    }

    pitch = ROUND_TO_MULTIPLE (width * cpp, 256);
    if (hdr_size) {
        nr_hdr_lines = hdr_size / pitch;
        if (hdr_size % pitch)
            nr_hdr_lines++;
    }

    bo = drm_intel_bo_alloc_for_render (driver->manager,
            "surface", (height + nr_hdr_lines) * pitch, 0);

    if (bo == NULL) {
        _DBG_PRINTF ("Could not allocate GEM object for surface buffer: "
                "width (%d), height (%d), (pitch: %d): %m\n",
                width, height, pitch);
        return NULL;
    }

    buffer = i915_create_buffer_helper (driver, bo);
    if (buffer == NULL) {
        drm_intel_bo_unreference (bo);
        return NULL;
    }

    buffer->base.prime_fd = -1;
    buffer->base.name = 0;
    buffer->base.fb_id = 0;
    buffer->base.drm_format = drm_format;
    buffer->base.bpp = bpp;
    buffer->base.cpp = cpp;
    buffer->base.scanout = IS_SURFACE_FOR_SCANOUT(flags) ? 1 : 0;
    buffer->base.width = width;
    buffer->base.height = height;
    buffer->base.pitch = pitch;
    buffer->base.offset = nr_hdr_lines * pitch;
    buffer->base.vaddr = NULL;

    _DBG_PRINTF ("Allocate GEM object for surface buffer: "
            "width (%d), height (%d), (pitch: %d), size (%lu), offset (%ld)\n",
            buffer->base.width, buffer->base.height, buffer->base.pitch,
            buffer->base.size, buffer->base.offset);

    return &buffer->base;
}

#ifdef DRM_INTEL_HAVE_CREATE_FROM_HANDLE

static inline drm_intel_bo * create_bo_from_handle (DrmDriver *driver,
        uint32_t handle, size_t size)
{
    char name [64];
    sprintf(name, "buffer %u", handle);
    return drm_intel_bo_gem_create_from_handle (driver->manager,
            name, handle, size);
}

#else   /* defined HAVE_CREATE_FROM_HANDLE */

static drm_intel_bo * create_bo_from_handle (DrmDriver *driver,
        uint32_t handle, size_t size)
{
    (void)size;
    struct drm_gem_flink flink_request;
    char name [64];
    drm_intel_bo *bo;

    /* FIXME: This can't be the right way to do this.
     *
     * 1) It requires skirting around the API and using ioctls
     * 2) It requires taking a local handle, turning it into a
     * a global handle ("name"), just so we can use an api that
     * will open the global name and grab the local handle from it.
     */

    memset (&flink_request, 0, sizeof (struct drm_gem_flink));
    flink_request.handle = handle;

    sprintf(name, "buffer %u", handle);

    if (ioctl (driver->device_fd, DRM_IOCTL_GEM_FLINK, &flink_request) < 0)
    {
        _DBG_PRINTF ("Could not export global name for handle %u", handle);
        return NULL;
    }

    bo = drm_intel_bo_gem_create_from_name (driver->manager,
            name, flink_request.name);

    return bo;
}
#endif  /* not defined HAVE_CREATE_FROM_HANDLE */

static int check_format_size(size_t size,
        uint32_t drm_format, uint32_t hdr_size,
        uint32_t width, uint32_t height, uint32_t pitch)
{
    int bpp, cpp;
    uint32_t nr_hdr_lines = 0;

    if (drm_format_to_bpp(drm_format, &bpp, &cpp) == 0) {
        _ERR_PRINTF("DRM>i915: not supported format: %d\n", drm_format);
        return -1;
    }

    if (pitch != ROUND_TO_MULTIPLE(width * cpp, 256)) {
        _ERR_PRINTF("DRM>i915: bad pitch value: %u\n", pitch);
        return -1;
    }

    if (hdr_size) {
        nr_hdr_lines = hdr_size / pitch;
        if (hdr_size % pitch)
            nr_hdr_lines++;
    }

    if (size && size != (height + nr_hdr_lines) * pitch) {
        _ERR_PRINTF("DRM>i915: bad size: %lu\n", (unsigned long)size);
        return -1;
    }

    return 0;
}

static DrmSurfaceBuffer* i915_create_buffer_from_handle (DrmDriver *driver,
        uint32_t handle, size_t size,
        uint32_t drm_format, uint32_t hdr_size,
        uint32_t width, uint32_t height, uint32_t pitch)
{
    drm_intel_bo *bo;
    my_surface_buffer *buffer;

    if (check_format_size(size, drm_format, hdr_size, width, height, pitch)) {
        _ERR_PRINTF("DRM>i915: bad surface parameters for handle %u: "
                "whole size: %lu, header size: %u, %u x %u, pitch: %u\n",
                handle, (unsigned long)size, hdr_size, width, height, pitch);
        return NULL;
    }

    bo = create_bo_from_handle (driver, handle, size);
    if (bo == NULL) {
        _ERR_PRINTF ("DRM>i915: could not open GEM object with handle %u: %m\n",
                handle);
        return NULL;
    }

    buffer = i915_create_buffer_helper (driver, bo);
    if (buffer == NULL) {
        drm_intel_bo_unreference (bo);
        return NULL;
    }

    buffer->base.prime_fd = -1;
    buffer->base.name = 0;
    buffer->base.fb_id = 0;
    return &buffer->base;
}

static DrmSurfaceBuffer* i915_create_buffer_from_name (DrmDriver *driver,
        uint32_t name, uint32_t drm_format, uint32_t hdr_size,
        uint32_t width, uint32_t height, uint32_t pitch)
{
    drm_intel_bo *bo;
    my_surface_buffer *buffer;
    char sz_name [64];

    if (check_format_size(0, drm_format, hdr_size, width, height, pitch)) {
        _ERR_PRINTF("DRM>i915: bad surface parameters for name %u: "
                "whole size: <unknown>, header size: %u, %u x %u, pitch: %u\n",
                name, hdr_size, width, height, pitch);
        return NULL;
    }

    sprintf(sz_name, "buffer %u", name);
    bo = drm_intel_bo_gem_create_from_name (driver->manager,
            sz_name, name);
    if (bo == NULL) {
        _ERR_PRINTF ("DRM>i915: could not open GEM object with name %u: %m\n",
                name);
        return NULL;
    }

    buffer = i915_create_buffer_helper (driver, bo);
    if (buffer == NULL) {
        drm_intel_bo_unreference (bo);
        return NULL;
    }

    buffer->base.prime_fd = -1;
    buffer->base.name = name;
    buffer->base.fb_id = 0;
    return &buffer->base;
}

static DrmSurfaceBuffer* i915_create_buffer_from_prime_fd (DrmDriver *driver,
        int prime_fd, size_t size,
        uint32_t drm_format, uint32_t hdr_size,
        uint32_t width, uint32_t height, uint32_t pitch)
{
    drm_intel_bo *bo;
    my_surface_buffer *buffer;

    if (drm_format &&
            check_format_size(size, drm_format, hdr_size, width, height, pitch)) {
        _ERR_PRINTF("DRM>i915: bad surface parameters for prime fd %d: "
                "whole size: %lu, header size: %u, %u x %u, pitch: %u\n",
                prime_fd, (unsigned long)size, hdr_size, width, height, pitch);
        return NULL;
    }

    bo = drm_intel_bo_gem_create_from_prime (driver->manager,
            prime_fd, size);
    if (bo == NULL) {
        _ERR_PRINTF ("DRM>i915: cannot create GEM object with fd (%d): %m\n",
                prime_fd);
        return NULL;
    }

    buffer = i915_create_buffer_helper (driver, bo);
    if (buffer == NULL) {
        drm_intel_bo_unreference (bo);
        return NULL;
    }

    buffer->base.prime_fd = prime_fd;
    buffer->base.name = 0;
    buffer->base.fb_id = 0;
    return &buffer->base;
}

static uint8_t* i915_map_buffer (DrmDriver *driver,
        DrmSurfaceBuffer* buffer)
{
    (void)driver;
    my_surface_buffer *my_buffer = (my_surface_buffer *)buffer;

    assert (my_buffer != NULL);
    assert (my_buffer->base.vaddr == NULL);

    if (buffer->scanout) {
        drm_intel_gem_bo_map_gtt (my_buffer->bo);
    }
    else {
        drm_intel_bo_map (my_buffer->bo, 1);
    }

    my_buffer->base.vaddr = my_buffer->bo->virtual;
    return my_buffer->base.vaddr;
}

static void i915_unmap_buffer (DrmDriver *driver,
        DrmSurfaceBuffer* buffer)
{
    (void)driver;
    my_surface_buffer *my_buffer = (my_surface_buffer *)buffer;
    assert (my_buffer != NULL);
    assert (my_buffer->base.vaddr != NULL);

    if (buffer->scanout)
        drm_intel_gem_bo_unmap_gtt (my_buffer->bo);
    else
        drm_intel_bo_unmap (my_buffer->bo);

    my_buffer->base.vaddr = NULL;
}

static void i915_destroy_buffer (DrmDriver *driver,
        DrmSurfaceBuffer* buffer)
{
    my_surface_buffer *my_buffer = (my_surface_buffer *)buffer;
    assert (my_buffer != NULL);

    if (my_buffer->base.vaddr) {
        if (buffer->scanout)
            drm_intel_gem_bo_unmap_gtt (my_buffer->bo);
        else
            drm_intel_bo_unmap (my_buffer->bo);
    }

    drm_intel_bo_unreference (my_buffer->bo);
    free (my_buffer);

    driver->nr_buffers--;

    _DBG_PRINTF("Buffer object (%u) destroied\n", my_buffer->base.handle);
}


static inline unsigned int translate_raster_op(ColorLogicalOp logicop)
{
    unsigned int op = logicop >> COLOR_LOGICOP_SHIFT;
    return op | (op << 4);
}

static inline uint32_t br13_for_cpp(int cpp)
{
    switch (cpp) {
        case 4:
            return BR13_8888;
            break;
        case 2:
            return BR13_565;
            break;
        case 1:
            return BR13_8;
            break;
        default:
            assert(0);
            return 0;
    }
}

static int i915_fill_rect (DrmDriver *driver,
        DrmSurfaceBuffer* dst_buf, const GAL_Rect* rc, uint32_t clear_value)
{
    (void)driver;
    my_surface_buffer *buffer;
    drm_intel_bo *aper_array[2];
    uint32_t BR13, CMD;
    int x1, y1, x2, y2;

    buffer = (my_surface_buffer*)dst_buf;
    assert (buffer != NULL);

    BR13 = 0xf0 << 16;
    CMD = XY_COLOR_BLT_CMD;

    /* Setup the blit command */
    if (buffer->base.cpp == 4) {
        /* clearing RGBA */
        CMD |= XY_BLT_WRITE_ALPHA | XY_BLT_WRITE_RGB;
    }

    _DBG_PRINTF ("buffer info: pitch (%u), cpp (%u), width (%u), height (%u)\n",
            buffer->base.pitch, buffer->base.cpp,
            buffer->base.width, buffer->base.height);

    BR13 |= buffer->base.pitch;
    BR13 |= br13_for_cpp(buffer->base.cpp);

    x1 = rc->x;
    y1 = rc->y;
    x2 = rc->x + rc->w;
    y2 = rc->y + rc->h;

#if 0
    if (dst_buf->offset) {
        int nr_lines = dst_buf->offset/dst_buf->pitch;
        y1 += nr_lines;
        y2 += nr_lines;
    }
#endif

    assert(x1 < x2);
    assert(y1 < y2);

    /* do space check before going any further */
    aper_array[0] = driver->batch.bo;
    aper_array[1] = buffer->bo;

    if (drm_intel_bufmgr_check_aperture_space(aper_array,
                TABLESIZE(aper_array)) != 0) {
        intel_batchbuffer_flush(driver,
                (driver->gen > 4) ? I915_EXEC_BLT : I915_EXEC_RENDER);
    }

    intel_batchbuffer_begin(driver, 6);
    intel_batchbuffer_emit_dword(driver, CMD | (6 - 2));
    intel_batchbuffer_emit_dword(driver, BR13);
    intel_batchbuffer_emit_dword(driver, (y1 << 16) | x1);
    intel_batchbuffer_emit_dword(driver, (y2 << 16) | x2);
    intel_batchbuffer_emit_reloc_fenced(driver, buffer->bo,
            I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER,
            dst_buf->offset);
    intel_batchbuffer_emit_dword(driver, clear_value);
    intel_batchbuffer_advance(driver);
    intel_batchbuffer_flush(driver,
            (driver->gen > 4) ? I915_EXEC_BLT : I915_EXEC_RENDER);

    intel_batchbuffer_emit_mi_flush(driver);
    return 0;
}

static int i915_do_blit(DrmDriver *driver,
        DrmSurfaceBuffer* src_buf, const GAL_Rect* src_rc,
        DrmSurfaceBuffer* dst_buf, const GAL_Rect* dst_rc,
        const DrmBlitOperations *ops)
{
    my_surface_buffer *buffer;
    unsigned int cpp;
    int src_pitch;
    drm_intel_bo *src_bo;
    unsigned int src_offset = src_buf->offset;
    int dst_pitch;
    drm_intel_bo *dst_bo;
    unsigned int dst_offset = dst_buf->offset;
    int src_x = src_rc->x, src_y = src_rc->y;
    int dst_x = dst_rc->x, dst_y = dst_rc->y;
    int w = src_rc->w, h = src_rc->h;

    unsigned int CMD, BR13, pass;
    int dst_x2 = dst_x + w;
    int dst_y2 = dst_y + h;
    drm_intel_bo *aper_array[3];

    buffer = (my_surface_buffer*)src_buf;
    assert (buffer != NULL);
    src_bo = buffer->bo;
    src_pitch = buffer->base.pitch;
    cpp = buffer->base.cpp;

    buffer = (my_surface_buffer*)dst_buf;
    assert (buffer != NULL);
    dst_bo = buffer->bo;
    dst_pitch = buffer->base.pitch;

    /* do space check before going any further */
    pass = 0;
    do {
        aper_array[0] = driver->batch.bo;
        aper_array[1] = dst_bo;
        aper_array[2] = src_bo;

        if (dri_bufmgr_check_aperture_space(aper_array, 3) != 0) {
            intel_batchbuffer_flush(driver,
                    (driver->gen > 4) ? I915_EXEC_BLT : I915_EXEC_RENDER);
            pass++;
        } else
            break;
    } while (pass < 2);

    if (pass >= 2)
        return -1;

    intel_batchbuffer_require_space(driver, 8 * 4);

    _DBG_PRINTF("src:buf(%p)/%d+%d %d,%d dst:buf(%p)/%d+%d %d,%d sz:%dx%d\n",
            src_bo, src_pitch, src_offset, src_x, src_y,
            dst_bo, dst_pitch, dst_offset, dst_x, dst_y, w, h);

    _DBG_PRINTF("src cpp: %d src offset(%d)\n", cpp, src_offset);

    /* Blit pitch must be dword-aligned.  Otherwise, the hardware appears to drop
     * the low bits.  Offsets must be naturally aligned.
     */
    if (src_pitch % 4 != 0 || dst_pitch % 4 != 0) {
        _WRN_PRINTF("pitches are not dword-aligned: src(%d), dst(%d)\n",
                src_pitch, dst_pitch);
        return -1;
    }

    BR13 = br13_for_cpp(cpp) |
        translate_raster_op(ops ? ops->rop : COLOR_LOGICOP_COPY) << 16;

    switch (cpp) {
        case 1:
        case 2:
            CMD = XY_SRC_COPY_BLT_CMD;
            break;
        case 4:
            CMD = XY_SRC_COPY_BLT_CMD | XY_BLT_WRITE_ALPHA | XY_BLT_WRITE_RGB;
            break;
        default:
            assert(0);
            return -1;
    }

    if (dst_y2 <= dst_y || dst_x2 <= dst_x) {
        _WRN_PRINTF("bad destination rectangle: (%d, %d, %d, %d)\n",
                dst_x, dst_y, dst_x2, dst_y2);
        return -1;
    }

    assert(dst_x < dst_x2);
    assert(dst_y < dst_y2);

    intel_batchbuffer_begin(driver, 8);

    intel_batchbuffer_emit_dword(driver, CMD | (8 - 2));
    intel_batchbuffer_emit_dword(driver, BR13 | (uint16_t)dst_pitch);
    intel_batchbuffer_emit_dword(driver, (dst_y << 16) | dst_x);
    intel_batchbuffer_emit_dword(driver, (dst_y2 << 16) | dst_x2);
    intel_batchbuffer_emit_reloc_fenced(driver, dst_bo,
            I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER,
            dst_offset);
    intel_batchbuffer_emit_dword(driver, (src_y << 16) | src_x);
    intel_batchbuffer_emit_dword(driver, (uint16_t)src_pitch);
    intel_batchbuffer_emit_reloc_fenced(driver, src_bo,
            I915_GEM_DOMAIN_RENDER, 0,
            src_offset);
    intel_batchbuffer_advance(driver);
    intel_batchbuffer_flush(driver,
            (driver->gen > 4) ? I915_EXEC_BLT : I915_EXEC_RENDER);

    intel_batchbuffer_emit_mi_flush(driver);
    return 0;
}

static int i915_check_blit (DrmDriver *driver,
        DrmSurfaceBuffer* src_buf, const GAL_Rect *srcrc,
        DrmSurfaceBuffer* dst_buf, const GAL_Rect *dstrc,
        const DrmBlitOperations *ops)
{
    (void)driver;
    drm_intel_bo *src_bo, *dst_bo;
    uint32_t src_tiling_mode, src_swizzle_mode;
    uint32_t dst_tiling_mode, dst_swizzle_mode;

    /* TODO: only copy supprted so far. */
    if (srcrc->w != srcrc->h || srcrc->h != dstrc->h ||
            ops->cpy != BLIT_COPY_TRANSLATE ||
            ops->key != BLIT_COLORKEY_NONE ||
            ops->alf != BLIT_ALPHA_NONE ||
            (ops->bld != COLOR_BLEND_LEGACY &&
             ops->bld != COLOR_BLEND_PD_SRC_OVER)) {
        goto failed;
    }

    src_bo = ((my_surface_buffer*)src_buf)->bo;
    dst_bo = ((my_surface_buffer*)dst_buf)->bo;
    drm_intel_bo_get_tiling(src_bo, &src_tiling_mode, &src_swizzle_mode);
    drm_intel_bo_get_tiling(dst_bo, &dst_tiling_mode, &dst_swizzle_mode);

    if (src_tiling_mode == dst_tiling_mode &&
            src_swizzle_mode == dst_swizzle_mode &&
            src_buf->drm_format == dst_buf->drm_format)
        return 0;

failed:
    _DBG_PRINTF("CANNOT blit src_buf(%p) to dst_buf(%p)\n",
            src_buf, dst_buf);
    return -1;
}

static int i915_copy_buff(DrmDriver *driver,
        DrmSurfaceBuffer *src_buf, const GAL_Rect *src_rc,
        DrmSurfaceBuffer *dst_buf, const GAL_Rect *dst_rc,
        BlitCopyOperation op)
{
    if (src_rc->w != dst_rc->w || src_rc->h != dst_rc->h ||
            op != BLIT_COPY_TRANSLATE) {
        return -1;
    }

    return i915_do_blit(driver, src_buf, src_rc, dst_buf, dst_rc, NULL);
}

DrmDriverOps* _drm_device_get_i915_driver(int device_fd)
{
    (void)device_fd;

    static DrmDriverOps i915_driver = {
        .create_driver = i915_create_driver,
        .destroy_driver = i915_destroy_driver,
        .flush = i915_flush,
        .create_buffer = i915_create_buffer,
        .create_buffer_from_handle = i915_create_buffer_from_handle,
        .create_buffer_from_name = i915_create_buffer_from_name,
        .create_buffer_from_prime_fd = i915_create_buffer_from_prime_fd,
        .map_buffer = i915_map_buffer,
        .unmap_buffer = i915_unmap_buffer,
        .destroy_buffer = i915_destroy_buffer,
        .fill_rect = i915_fill_rect,
        .check_blit = i915_check_blit,
        .do_blit = i915_do_blit,
        .copy_buff = i915_copy_buff,
    };

    return &i915_driver;
}

#else /* HAVE_DRM_INTEL */

DrmDriverOps* __drm_ex_driver_get(const char* driver_name,int device_fd,
        int* version)
{
    _WRN_PRINTF("This external DRM is a NULL implementation!");
    return NULL;
}

#endif /* !HAVE_DRM_INTEL */

#endif /* _MGGAL_DRM */
