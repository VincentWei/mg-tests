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
** mydridriver.c: A sample DRM driver for MiniGUI 4.0.
**
** This driver is derived from early version of Plymouth and Mesa.
**
** Copyright (C) 2019 FMSoft (http://www.fmsoft.cn).
**
** Copyright notice of Plymouth:
**
** Copyright (C) 2009 Red Hat, Inc.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2, or (at your option)
** any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
** 02111-1307, USA.
**
** Written by: Ray Strode <rstrode@redhat.com>
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

#ifdef __TARGET_EXTERNAL__

#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/exstubs.h>

#ifdef _MGGAL_DRI

#ifdef HAVE_DRM_INTEL

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <drm.h>
#include <drm_fourcc.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <i915_drm.h>
#include <libdrm/intel_bufmgr.h>

#include "intel_reg.h"
#include "intel_context.h"
#include "intel_batchbuffer.h"

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
        _ERR_PRINTF("drm_intel_bo_subdata failed: %s\n", strerror(-ret));
    }

    if (ret != 0) {
        _ERR_PRINTF("intel_do_flush_locked failed(%p): %s\n",
                batch->bo, strerror(-ret));
        assert(0);
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

#if 0
static bool intel_batchbuffer_emit_reloc(struct _DrmDriver *driver,
        drm_intel_bo *buffer,
        uint32_t read_domains, uint32_t write_domain,
        uint32_t delta)
{
    int ret;

    ret = drm_intel_bo_emit_reloc(driver->batch.bo, 4*driver->batch.used,
            buffer, delta,
            read_domains, write_domain);
    assert(ret == 0);
    (void)ret;

    /*
     * Using the old buffer offset, write in what the right data would be, in case
     * the buffer doesn't move and we can short-circuit the relocation processing
     * in the kernel
     */
    intel_batchbuffer_emit_dword(driver, buffer->offset + delta);

    return true;
}
#endif

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

#if 0
static void intel_batchbuffer_data(struct _DrmDriver *driver,
        const void *data, GLuint bytes)
{
    assert((bytes & 3) == 0);
    intel_batchbuffer_require_space(driver, bytes);
    memcpy(driver->batch.map + driver->batch.used, data, bytes);
    driver->batch.used += bytes >> 2;
}
#endif

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
    intel_batchbuffer_flush(driver, I915_EXEC_RENDER);
}

static DrmDriver* i915_create_driver (int device_fd)
{
    DrmDriver *driver;

    driver = calloc (1, sizeof (DrmDriver));
    driver->device_fd = device_fd;

    driver->manager = drm_intel_bufmgr_gem_init (driver->device_fd, BATCH_SZ);
    if (driver->manager == NULL) {
        _ERR_PRINTF ("%s: failed to initialize the Intel buffer manager\n",
            __func__);
        free (driver);
        return NULL;
    }

    drm_intel_bufmgr_gem_enable_fenced_relocs(driver->manager);

#ifdef WITH_BUFFER_MANAGEMENT
    driver->buffers = ply_hashtable_new (ply_hashtable_direct_hash,
            ply_hashtable_direct_compare);
#endif
    driver->nr_buffers = 0;

    driver->maxBatchSize = BATCH_SIZE;
    intel_batchbuffer_init(driver);

    return driver;
}

static void i915_destroy_driver (DrmDriver *driver)
{
    _DBG_PRINTF ("%s: destroying driver buffer manager: %d buffers left\n",
            __func__, driver->nr_buffers);

#ifdef WITH_BUFFER_MANAGEMENT
    ply_hashtable_free (driver->buffers);
#endif
    intel_batchbuffer_free(driver);
    drm_intel_bufmgr_destroy (driver->manager);
    free (driver);
}

static void i915_flush_driver (DrmDriver *driver)
{
    intel_batchbuffer_emit_mi_flush(driver);
}

typedef struct _my_surface_buffer {
    DrmSurfaceBuffer base;

    drm_intel_bo *bo;

    uint32_t added_fb:1;
} my_surface_buffer;

#define ROUND_TO_MULTIPLE(n, m) (((n) + (((m) - 1))) & ~((m) - 1))

static my_surface_buffer * drm_buffer_new (DrmDriver *driver,
        drm_intel_bo *buffer_object,
        uint32_t id,
        uint32_t width,
        uint32_t height,
        uint32_t pitch)
{
    my_surface_buffer *buffer;

    buffer = calloc (1, sizeof (my_surface_buffer));
    buffer->base.handle = buffer_object->handle;
    buffer->base.id = id;
    buffer->base.width = width;
    buffer->base.height = height;
    buffer->base.pitch = pitch;
    buffer->base.size = buffer_object->size;

    buffer->bo = buffer_object;
    _DBG_PRINTF ("returning %ux%u buffer with stride %u\n",
            width, height, pitch);

    return buffer;
}

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

/* 24 bpp is not supported by i915 driver
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
            drm_intel_bo *buffer_object,
            uint32_t drm_format,
            unsigned int width, unsigned int height,
            unsigned int pitch)
{
    uint32_t buffer_id;
    uint32_t handles[4], pitches[4], offsets[4];
    my_surface_buffer *buffer;

    handles[0] = buffer_object->handle;
    pitches[0] = pitch;
    offsets[0] = 0;

    if (drmModeAddFB2(driver->device_fd, width, height, drm_format,
            handles, pitches, offsets, &buffer_id, 0) != 0) {
        drm_intel_bo_unreference (buffer_object);
        return 0;
    }

    buffer = drm_buffer_new (driver,
            buffer_object, buffer_id, width, height, pitch);

#ifdef WITH_BUFFER_MANAGEMENT
    ply_hashtable_insert (driver->buffers,
            (void *) (uintptr_t) buffer_id, buffer);
#endif
    driver->nr_buffers++;

    _DBG_PRINTF("%s: Buffer object (%u) created\n", __func__, buffer_id);
    return buffer;
}

static DrmSurfaceBuffer* i915_create_buffer (DrmDriver *driver,
            uint32_t drm_format,
            unsigned int width, unsigned int height)
{
    drm_intel_bo *buffer_object;
    my_surface_buffer *buffer;
    int bpp, cpp;
    unsigned int pitch;

    if (drm_format_to_bpp(drm_format, &bpp, &cpp) == 0) {
        _ERR_PRINTF ("Not supported DRM format: %d\n", drm_format);
        return NULL;
    }

    pitch = ROUND_TO_MULTIPLE (width * cpp, 256);
    buffer_object = drm_intel_bo_alloc_for_render (driver->manager,
            "surface", height * pitch, 0);

    if (buffer_object == NULL) {
        _ERR_PRINTF ("Could not allocate GEM object for frame buffer (pitch: %d): %m\n",
            pitch);
        return NULL;
    }

    buffer = i915_create_buffer_helper (driver, buffer_object, drm_format,
            width, height, pitch);
    if (buffer == NULL) {
        _ERR_PRINTF ("Could not set up GEM object as frame buffer (%dx%d-%dbpp) 0x%08x: %m\n",
            width, height, bpp, drm_format);
        return NULL;
    }

    buffer->base.bpp = bpp;
    buffer->base.cpp = cpp;
    buffer->added_fb = 1;
    return &buffer->base;
}

static DrmSurfaceBuffer* i915_create_buffer_from_handle (DrmDriver *driver,
            uint32_t handle, unsigned long size, uint32_t drm_format,
            unsigned int width, unsigned int height, unsigned int pitch)
{
#if 0 // TODO: add drm_intel_bo_gem_create_from_handle
    drm_intel_bo *buffer_object;
    my_surface_buffer *buffer;
    int bpp, cpp;

    if (drm_format_to_bpp(drm_format, &bpp, &cpp) == 0) {
        _ERR_PRINTF ("Not supported DRM format: %d\n", drm_format);
        return NULL;
    }

    buffer_object = drm_intel_bo_gem_create_from_handle (driver->manager,
            handle, size);
    if (buffer_object == NULL) {
        _ERR_PRINTF ("Could not open GEM object with handle %u for frame buffer: %m\n",
                handle);
        return NULL;
    }

    if (buffer_object->size < height * pitch) {
        _ERR_PRINTF ("Specified surface size is larger than the size of the buffer object: %u.\n",
                handle);
        drm_intel_bo_unreference (buffer_object);
        return NULL;
    }

    buffer = i915_create_buffer_helper (driver, buffer_object, drm_format,
            width, height, pitch);
    if (buffer == NULL) {
        _ERR_PRINTF ("Could not set up GEM object as frame buffer (%dx%d-%dbpp) 0x%08x: %m\n",
            width, height, bpp, drm_format);
        return NULL;
    }

    buffer->base.bpp = bpp;
    buffer->base.cpp = cpp;
    buffer->added_fb = 1;
    return &buffer->base;
#else
    return NULL;
#endif
}

static DrmSurfaceBuffer* i915_create_buffer_from_name (DrmDriver *driver,
            uint32_t name, uint32_t drm_format,
            unsigned int width, unsigned int height, unsigned int pitch)
{
    drm_intel_bo *buffer_object;
    my_surface_buffer *buffer;
    char sz_name [64];
    int bpp, cpp;

    if (drm_format_to_bpp(drm_format, &bpp, &cpp) == 0) {
        _ERR_PRINTF ("Not supported DRM format: %d\n", drm_format);
        return NULL;
    }

    sprintf(sz_name, "buffer %u", name);
    buffer_object = drm_intel_bo_gem_create_from_name (driver->manager,
            sz_name, name);
    if (buffer_object == NULL) {
        _ERR_PRINTF ("Could not open GEM object with name %u for frame buffer: %m\n",
                name);
        return NULL;
    }

    if (buffer_object->size < height * pitch) {
        _ERR_PRINTF ("Specified surface size is larger than the size of the buffer object: %u.\n",
                name);
        drm_intel_bo_unreference (buffer_object);
        return NULL;
    }

    buffer = i915_create_buffer_helper (driver, buffer_object, drm_format,
            width, height, pitch);
    if (buffer == NULL) {
        _ERR_PRINTF ("Could not set up GEM object as frame buffer (%dx%d-%dbpp) 0x%08x: %m\n",
            width, height, bpp, drm_format);
        return NULL;
    }

    buffer->base.bpp = bpp;
    buffer->base.cpp = cpp;
    buffer->added_fb = 1;
    return &buffer->base;
}

static DrmSurfaceBuffer* i915_create_buffer_from_prime_fd (DrmDriver *driver,
            int prime_fd, unsigned long size, uint32_t drm_format,
            unsigned int width, unsigned int height, unsigned int pitch)
{
    drm_intel_bo *buffer_object;
    my_surface_buffer *buffer;
    int bpp, cpp;

    if (drm_format_to_bpp(drm_format, &bpp, &cpp) == 0) {
        _ERR_PRINTF ("Not supported DRM format: %d\n", drm_format);
        return NULL;
    }

    buffer_object = drm_intel_bo_gem_create_from_prime (driver->manager,
            prime_fd, size);
    if (buffer_object == NULL) {
        _ERR_PRINTF ("Could not open GEM object with prime fd (%d) for frame buffer: %m\n",
                prime_fd);
        return NULL;
    }

    if (buffer_object->size < height * pitch) {
        _ERR_PRINTF ("Specified surface size is larger than the size of the buffer object: %u.\n",
                buffer_object->handle);
        drm_intel_bo_unreference (buffer_object);
        return NULL;
    }

    buffer = i915_create_buffer_helper (driver, buffer_object, drm_format,
            width, height, pitch);
    if (buffer == NULL) {
        _ERR_PRINTF ("Could not set up GEM object as frame buffer (%dx%d-%dbpp) 0x%08x: %m\n",
            width, height, bpp, drm_format);
        return NULL;
    }

    buffer->base.bpp = bpp;
    buffer->base.cpp = cpp;
    buffer->added_fb = 1;
    return &buffer->base;
}

#if 0
static my_surface_buffer *get_buffer_from_id (DrmDriver *driver,
        uint32_t buffer_id)
{
    static my_surface_buffer *buffer;

    buffer = ply_hashtable_lookup (driver->buffers,
            (void *) (uintptr_t) buffer_id);

    return buffer;
}

static drm_intel_bo * create_intel_bo_from_handle (DrmDriver *driver,
        uint32_t handle)
{
    struct drm_gem_flink flink_request;
    char name [64];
    drm_intel_bo *buffer_object;

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

    buffer_object = drm_intel_bo_gem_create_from_name (driver->manager,
            name, flink_request.name);

    return buffer_object;
}

static my_surface_buffer* drm_buffer_new_from_id (DrmDriver *driver,
        uint32_t buffer_id)
{
    my_surface_buffer *buffer;
    drmModeFB *fb;
    drm_intel_bo *buffer_object;

    fb = drmModeGetFB (driver->device_fd, buffer_id);

    if (fb == NULL)
    {
        _DBG_PRINTF ("could not get FB with buffer id %u", buffer_id);
        return NULL;
    }

    buffer_object = create_intel_bo_from_handle (driver, fb->handle);

    if (buffer_object == NULL)
    {
        _DBG_PRINTF ("could not create buffer object from handle %lu",
                (unsigned long) fb->handle);
        drmModeFreeFB (fb);
        return NULL;
    }

    buffer = drm_buffer_new (driver, buffer_object, buffer_id,
            fb->width, fb->height, fb->pitch);
    drmModeFreeFB (fb);

    return buffer;
}

static BOOL i915_fetch_buffer (DrmDriver *driver,
            uint32_t buffer_id,
            unsigned int *width, unsigned int *height,
            unsigned int *pitch)
{
    my_surface_buffer *buffer;

    buffer = get_buffer_from_id (driver, buffer_id);

    if (buffer == NULL)
    {
        _DBG_PRINTF ("could not fetch buffer %u, creating one\n", buffer_id);
        buffer = drm_buffer_new_from_id (driver, buffer_id);

        if (buffer == NULL)
        {
            _DBG_PRINTF ("could not create buffer either %u\n", buffer_id);
            return FALSE;
        }

        ply_hashtable_insert (driver->buffers,
                (void *) (uintptr_t) buffer_id,
                buffer);
        driver->nr_buffers++;
    }

    if (width != NULL)
        *width = buffer->base.width;

    if (height != NULL)
        *height = buffer->base.height;

    if (pitch != NULL)
        *pitch = buffer->base.pitch;

    _DBG_PRINTF ("fetched %ux%u buffer with stride %u\n",
            buffer->base.width, buffer->base.height, buffer->base.pitch);
    return TRUE;
}
#endif

static uint8_t* i915_map_buffer (DrmDriver *driver,
            DrmSurfaceBuffer* buffer)
{
    my_surface_buffer *my_buffer = (my_surface_buffer *)buffer;

    assert (my_buffer != NULL);
    assert (my_buffer->base.pixels == NULL);

    drm_intel_gem_bo_map_gtt (my_buffer->bo);
    my_buffer->base.pixels = my_buffer->bo->virtual;
    return my_buffer->base.pixels;
}

static void i915_unmap_buffer (DrmDriver *driver,
            DrmSurfaceBuffer* buffer)
{
    my_surface_buffer *my_buffer = (my_surface_buffer *)buffer;
    assert (my_buffer != NULL);
    assert (my_buffer->base.pixels != NULL);

    drm_intel_gem_bo_unmap_gtt (my_buffer->bo);
    my_buffer->base.pixels = NULL;
}

#if 0
static uint8_t * i915_begin_flush (DrmDriver *driver,
            uint32_t buffer_id)
{
    my_surface_buffer *buffer;

    buffer = get_buffer_from_id (driver, buffer_id);
    assert (buffer != NULL);

    return buffer->bo->virtual;
}

static void i915_end_flush (DrmDriver *driver,
            uint32_t buffer_id)
{
    my_surface_buffer *buffer;

    buffer = get_buffer_from_id (driver, buffer_id);
    assert (buffer != NULL);
}
#endif

static void i915_destroy_buffer (DrmDriver *driver,
            DrmSurfaceBuffer* buffer)
{
    my_surface_buffer *my_buffer = (my_surface_buffer *)buffer;
    assert (my_buffer != NULL);

    if (my_buffer->base.pixels) {
        drm_intel_gem_bo_unmap_gtt (my_buffer->bo);
        my_buffer->base.pixels = NULL;
    }

    if (my_buffer->added_fb)
        drmModeRmFB (driver->device_fd, my_buffer->base.id);

    drm_intel_bo_unreference (my_buffer->bo);

#ifdef WITH_BUFFER_MANAGEMENT
    ply_hashtable_remove (driver->buffers,
            (void *) (uintptr_t) my_buffer->base.id);
#endif
    driver->nr_buffers--;

    free (my_buffer);

    _DBG_PRINTF("%s: Buffer object (%u) destroied\n",
            __func__, buffer_id);
}

static unsigned int translate_raster_op(enum DrmColorLogicOp logicop)
{
   return logicop | (logicop << 4);
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

static int i915_clear_buffer (DrmDriver *driver,
            DrmSurfaceBuffer* dst_buf, const GAL_Rect* rc, uint32_t clear_value)
{
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

    BR13 |= buffer->base.pitch;
    BR13 |= br13_for_cpp(buffer->base.cpp);

    x1 = rc->x;
    y1 = rc->y;
    x2 = rc->x + rc->w;
    y2 = rc->y + rc->h;

    assert(x1 < x2);
    assert(y1 < y2);

    /* do space check before going any further */
    aper_array[0] = driver->batch.bo;
    aper_array[1] = buffer->bo;

    if (drm_intel_bufmgr_check_aperture_space(aper_array,
                TABLESIZE(aper_array)) != 0) {
        intel_batchbuffer_flush(driver, I915_EXEC_RENDER);
    }

    intel_batchbuffer_begin(driver, 6);
    intel_batchbuffer_emit_dword(driver, CMD | (6 - 2));
    intel_batchbuffer_emit_dword(driver, BR13);
    intel_batchbuffer_emit_dword(driver, (y1 << 16) | x1);
    intel_batchbuffer_emit_dword(driver, (y2 << 16) | x2);
    intel_batchbuffer_emit_reloc_fenced(driver, buffer->bo,
            I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER,
            0);
    intel_batchbuffer_emit_dword(driver, clear_value);
    intel_batchbuffer_advance(driver);
    intel_batchbuffer_flush(driver, I915_EXEC_BLT);

    intel_batchbuffer_emit_mi_flush(driver);
    return 0;
}

static int i915_check_blit (DrmDriver *driver,
            DrmSurfaceBuffer* src_buf, DrmSurfaceBuffer* dst_buf)
{
    if (src_buf->drm_format == dst_buf->drm_format)
        return 0;

    _DBG_PRINTF("%s: CANNOT blit src_buf(%p) to dst_buf(%p)\n",
            __func__, src_buf, dst_buf);
    return -1;
}

static int i915_copy_blit (DrmDriver *driver,
            DrmSurfaceBuffer* src_buf, const GAL_Rect* src_rc,
            DrmSurfaceBuffer* dst_buf, const GAL_Rect* dst_rc,
            enum DrmColorLogicOp logic_op)
{
    my_surface_buffer *buffer;
    unsigned int cpp;
    int src_pitch;
    drm_intel_bo *src_bo;
    unsigned int src_offset = 0;
    int dst_pitch;
    drm_intel_bo *dst_bo;
    unsigned int dst_offset = 0;
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
            intel_batchbuffer_flush(driver, I915_EXEC_BLT);
            pass++;
        } else
            break;
    } while (pass < 2);

    if (pass >= 2)
        return -1;

    intel_batchbuffer_require_space(driver, 8 * 4);

    _DBG_PRINTF("%s src:buf(%p)/%d+%d %d,%d dst:buf(%p)/%d+%d %d,%d sz:%dx%d\n",
            __func__,
            src_bo, src_pitch, src_offset, src_x, src_y,
            dst_bo, dst_pitch, dst_offset, dst_x, dst_y, w, h);

    /* Blit pitch must be dword-aligned.  Otherwise, the hardware appears to drop
     * the low bits.  Offsets must be naturally aligned.
     */
    if (src_pitch % 4 != 0 || dst_pitch % 4 != 0) {
        _DBG_PRINTF("%s pitches are not dword-aligned: src_pitch(%d), dst_pitch(%d)\n",
                __func__, src_pitch, dst_pitch);
        return -1;
    }

    BR13 = br13_for_cpp(cpp) | translate_raster_op(logic_op) << 16;

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
    intel_batchbuffer_flush(driver, I915_EXEC_BLT);

    intel_batchbuffer_emit_mi_flush(driver);
    return -1;
}

DrmDriverOps* __dri_ex_driver_get(const char* driver_name, int device_fd)
{
    _MG_PRINTF("%s called with driver name: %s\n", __func__, driver_name);

    if (strcmp(driver_name, "i915") == 0) {
        static DrmDriverOps i915_driver = {
            .create_driver = i915_create_driver,
            .destroy_driver = i915_destroy_driver,
            .flush_driver = i915_flush_driver,
            .create_buffer = i915_create_buffer,
            .create_buffer_from_handle = i915_create_buffer_from_handle,
            .create_buffer_from_name = i915_create_buffer_from_name,
            .create_buffer_from_prime_fd = i915_create_buffer_from_prime_fd,
            .map_buffer = i915_map_buffer,
            .unmap_buffer = i915_unmap_buffer,
            .destroy_buffer = i915_destroy_buffer,
            .clear_buffer = i915_clear_buffer,
            .check_blit = i915_check_blit,
            .copy_blit = i915_copy_blit,
        };

        return &i915_driver;
    }
    else {
        _MG_PRINTF("%s unknown DRM driver: %s\n", __func__, driver_name);
    }

    return NULL;
}

#else /* HAVE_DRM_INTEL */

DrmDriverOps* __dri_ex_driver_get(const char* driver_name, int device_fd)
{
    _WRN_PRINTF("This external DRM driver is a NULL implementation!");
    return NULL;
}

#endif /* !HAVE_DRM_INTEL */

#endif /* _MGGAL_DRI */

#endif /* __TARGET_EXTERNAL__ */
