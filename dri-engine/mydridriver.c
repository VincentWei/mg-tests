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
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <i915_drm.h>
#include <libdrm/intel_bufmgr.h>

#include "intel_reg.h"
#include "intel_context.h"
#include "intel_batchbuffer.h"

static DriDriver* i915_create_driver (int device_fd)
{
    DriDriver *driver;
    int page_size;

    driver = calloc (1, sizeof (DriDriver));
    driver->device_fd = device_fd;

    page_size = (int) sysconf (_SC_PAGE_SIZE);

    driver->manager = drm_intel_bufmgr_gem_init (driver->device_fd, page_size);
    if (driver->manager == NULL) {
        _ERR_PRINTF ("%s: failed to initialize the Intel buffer manager\n",
            __FUNCTION__);
        free (driver);
        return NULL;
    }

    driver->buffers = ply_hashtable_new (ply_hashtable_direct_hash,
            ply_hashtable_direct_compare);

    return driver;
}

static void i915_destroy_driver (DriDriver *driver)
{
    _DBG_PRINTF ("%s: destroying driver buffer manager\n",
            __FUNCTION__);

    ply_hashtable_free (driver->buffers);
    drm_intel_bufmgr_destroy (driver->manager);
    free (driver);
}

typedef struct _drm_surface_buffer
{
    drm_intel_bo *bo;
    uint32_t id;
    unsigned long width;
    unsigned long height;
    unsigned long pitch;

    uint32_t pixel_format:8;
    uint32_t depth:8;
    uint32_t bpp:8;
    uint32_t cpp:4;
    uint32_t added_fb:1;
} drm_surface_buffer;

#define ROUND_TO_MULTIPLE(n, m) (((n) + (((m) - 1))) & ~((m) - 1))

static drm_surface_buffer * drm_buffer_new (DriDriver *driver,
        drm_intel_bo *buffer_object,
        uint32_t id,
        unsigned long width,
        unsigned long height,
        unsigned long pitch)
{
    drm_surface_buffer *buffer;

    buffer = calloc (1, sizeof (drm_surface_buffer));
    buffer->bo = buffer_object;
    buffer->id = id;
    buffer->width = width;
    buffer->height = height;
    buffer->pitch = pitch;

    _DBG_PRINTF ("returning %lux%lu buffer with stride %lu",
            width, height, pitch);

    return buffer;
}

static drm_intel_bo * create_intel_bo_from_handle (DriDriver *driver,
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

static drm_surface_buffer* drm_buffer_new_from_id (DriDriver *driver,
        uint32_t buffer_id)
{
    drm_surface_buffer *buffer;
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

static drm_surface_buffer *get_buffer_from_id (DriDriver *driver,
        uint32_t buffer_id)
{
    static drm_surface_buffer *buffer;

    buffer = ply_hashtable_lookup (driver->buffers,
            (void *) (uintptr_t) buffer_id);

    return buffer;
}

static uint32_t i915_create_buffer (DriDriver *driver,
            enum DriPixelFormat pixel_format,
            unsigned int width, unsigned int height,
            unsigned int *pitch)
{
    drm_intel_bo *buffer_object;
    drm_surface_buffer *buffer;
    int depth, bpp, cpp;
    uint32_t buffer_id;

    switch (pixel_format) {
    case PIXEL_FORMAT_A8B8G8R8_UNORM:
    case PIXEL_FORMAT_X8B8G8R8_UNORM:
    case PIXEL_FORMAT_R8G8B8A8_UNORM:
    case PIXEL_FORMAT_R8G8B8X8_UNORM:
    case PIXEL_FORMAT_B8G8R8A8_UNORM:
    case PIXEL_FORMAT_B8G8R8X8_UNORM:
    case PIXEL_FORMAT_A8R8G8B8_UNORM:
    case PIXEL_FORMAT_X8R8G8B8_UNORM:
        depth = 24;
        bpp = 32;
        cpp = 4;
        break;
    case PIXEL_FORMAT_B5G6R5_UNORM:
    case PIXEL_FORMAT_R5G6B5_UNORM:
    case PIXEL_FORMAT_B4G4R4A4_UNORM:
    case PIXEL_FORMAT_B4G4R4X4_UNORM:
    case PIXEL_FORMAT_A4R4G4B4_UNORM:
    case PIXEL_FORMAT_A1B5G5R5_UNORM:
    case PIXEL_FORMAT_X1B5G5R5_UNORM:
    case PIXEL_FORMAT_B5G5R5A1_UNORM:
    case PIXEL_FORMAT_B5G5R5X1_UNORM:
    case PIXEL_FORMAT_A1R5G5B5_UNORM:
    case PIXEL_FORMAT_A4B4G4R4_UNORM:
    case PIXEL_FORMAT_R4G4B4A4_UNORM:
    case PIXEL_FORMAT_R5G5B5A1_UNORM:
        depth = 16;
        bpp = 16;
        cpp = 2;
        break;

    case PIXEL_FORMAT_B2G3R3_UNORM:
    case PIXEL_FORMAT_R3G3B2_UNORM:
        depth = 8;
        bpp = 8;
        cpp = 1;
        break;

    default:
        _ERR_PRINTF ("NEWGAL>DRI>I915: Not supported pixel format: %d\n", pixel_format);
        return 0;
    }

    *pitch = ROUND_TO_MULTIPLE (width * 4, 256);

    buffer_object = drm_intel_bo_alloc (driver->manager,
            "frame buffer",
            height * *pitch, 0);

    if (buffer_object == NULL) {
        _ERR_PRINTF ("Could not allocate GEM object for frame buffer: %m");
        return 0;
    }

    if (drmModeAddFB (driver->device_fd, width, height,
                depth, bpp, *pitch, buffer_object->handle,
                &buffer_id) != 0) {
        _ERR_PRINTF ("Could not set up GEM object as frame buffer: %m");
        drm_intel_bo_unreference (buffer_object);
        return 0;
    }

    buffer = drm_buffer_new (driver,
            buffer_object, buffer_id, width, height, *pitch);
    buffer->depth = depth;
    buffer->bpp = bpp;
    buffer->cpp = cpp;
    buffer->added_fb = 1;

    ply_hashtable_insert (driver->buffers,
            (void *) (uintptr_t) buffer_id,
            buffer);

    return buffer_id;
}

static BOOL i915_fetch_buffer (DriDriver *driver,
            uint32_t buffer_id,
            unsigned int *width, unsigned int *height,
            unsigned int *pitch)
{
    drm_surface_buffer *buffer;

    buffer = get_buffer_from_id (driver, buffer_id);

    if (buffer == NULL)
    {
        _DBG_PRINTF ("could not fetch buffer %u, creating one", buffer_id);
        buffer = drm_buffer_new_from_id (driver, buffer_id);

        if (buffer == NULL)
        {
            _DBG_PRINTF ("could not create buffer either %u", buffer_id);
            return FALSE;
        }

        ply_hashtable_insert (driver->buffers,
                (void *) (uintptr_t) buffer_id,
                buffer);
    }

    if (width != NULL)
        *width = buffer->width;

    if (height != NULL)
        *height = buffer->height;

    if (pitch != NULL)
        *pitch = buffer->pitch;

    _DBG_PRINTF ("fetched %lux%lu buffer with stride %lu",
            buffer->width, buffer->height, buffer->pitch);
    return TRUE;
}

static uint8_t* i915_map_buffer (DriDriver *driver,
            uint32_t buffer_id)
{
    drm_surface_buffer *buffer;

    buffer = get_buffer_from_id (driver, buffer_id);

    assert (buffer != NULL);
    drm_intel_gem_bo_map_gtt (buffer->bo);

    return buffer->bo->virtual;
}

static void i915_unmap_buffer (DriDriver *driver,
            uint32_t buffer_id)
{
    drm_surface_buffer *buffer;

    buffer = get_buffer_from_id (driver, buffer_id);
    assert (buffer != NULL);

    drm_intel_gem_bo_unmap_gtt (buffer->bo);
}

#if 0
static uint8_t * i915_begin_flush (DriDriver *driver,
            uint32_t buffer_id)
{
    drm_surface_buffer *buffer;

    buffer = get_buffer_from_id (driver, buffer_id);
    assert (buffer != NULL);

    return buffer->bo->virtual;
}

static void i915_end_flush (DriDriver *driver,
            uint32_t buffer_id)
{
    drm_surface_buffer *buffer;

    buffer = get_buffer_from_id (driver, buffer_id);
    assert (buffer != NULL);
}
#endif

static void i915_destroy_buffer (DriDriver *driver,
            uint32_t buffer_id)
{
    drm_surface_buffer *buffer;

    buffer = get_buffer_from_id (driver, buffer_id);

    assert (buffer != NULL);

    if (buffer->added_fb)
        drmModeRmFB (driver->device_fd, buffer->id);

    drm_intel_bo_unreference (buffer->bo);

    ply_hashtable_remove (driver->buffers,
            (void *) (uintptr_t) buffer_id);

    free (buffer);
}

static unsigned int translate_raster_op(enum DriColorLogicOp logicop)
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

static int i915_clear_buffer (DriDriver *driver,
            uint32_t buffer_id, const GAL_Rect* rc, uint32_t clear_value)
{
    drm_surface_buffer *buffer;
    drm_intel_bo *aper_array[2];
    uint32_t BR13, CMD;
    int x1, y1, x2, y2;

    buffer = get_buffer_from_id (driver, buffer_id);
    assert (buffer != NULL);

    BR13 = 0xf0 << 16;
    CMD = XY_COLOR_BLT_CMD;

    /* Setup the blit command */
    if (buffer->cpp == 4) {
        /* clearing RGBA */
        CMD |= XY_BLT_WRITE_ALPHA | XY_BLT_WRITE_RGB;
    }

    BR13 |= buffer->pitch;
    BR13 |= br13_for_cpp(buffer->cpp);

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
        intel_batchbuffer_flush(driver);
    }

    BEGIN_BATCH(6);
    OUT_BATCH(CMD | (6 - 2));
    OUT_BATCH(BR13);
    OUT_BATCH((y1 << 16) | x1);
    OUT_BATCH((y2 << 16) | x2);
    OUT_RELOC_FENCED(buffer->bo,
            I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER,
            0);
    OUT_BATCH(clear_value);
    ADVANCE_BATCH();

    if (driver->always_flush_cache)
        intel_batchbuffer_emit_mi_flush(driver);

    return 0;
}

static int i915_check_blit (DriDriver *driver,
            uint32_t src_id, uint32_t dst_id)
{
    return -1;
}


static int i915_copy_blit (DriDriver *driver,
            uint32_t src_id, const GAL_Rect* src_rc,
            uint32_t dst_id, const GAL_Rect* dst_rc)
{
    drm_surface_buffer *buffer;
    unsigned int cpp;
    int src_pitch;
    drm_intel_bo *src_buffer;
    unsigned int src_offset = 0;
    int dst_pitch;
    drm_intel_bo *dst_buffer;
    unsigned int dst_offset = 0;
    int src_x = src_rc->x, src_y = src_rc->y;
    int dst_x = dst_rc->x, dst_y = dst_rc->x;
    int w = src_rc->w, h = src_rc->w;
    enum DriColorLogicOp logic_op = COLOR_LOGICOP_COPY;

    unsigned int CMD, BR13, pass;
    int dst_x2 = dst_x + w;
    int dst_y2 = dst_y + h;
    drm_intel_bo *aper_array[3];
    BATCH_LOCALS;

    buffer = get_buffer_from_id (driver, src_id);
    assert (buffer != NULL);
    src_buffer = buffer->bo;
    src_pitch = buffer->pitch;
    cpp = buffer->cpp;

    buffer = get_buffer_from_id (driver, dst_id);
    assert (buffer != NULL);
    dst_buffer = buffer->bo;
    dst_pitch = buffer->pitch;

    /* do space check before going any further */
    pass = 0;
    do {
        aper_array[0] = driver->batch.bo;
        aper_array[1] = dst_buffer;
        aper_array[2] = src_buffer;

        if (dri_bufmgr_check_aperture_space(aper_array, 3) != 0) {
            intel_batchbuffer_flush(driver);
            pass++;
        } else
            break;
    } while (pass < 2);

    if (pass >= 2)
        return -1;

    intel_batchbuffer_require_space(driver, 8 * 4);
    _DBG_PRINTF("%s src:buf(%p)/%d+%d %d,%d dst:buf(%p)/%d+%d %d,%d sz:%dx%d\n",
            __func__,
            src_buffer, src_pitch, src_offset, src_x, src_y,
            dst_buffer, dst_pitch, dst_offset, dst_x, dst_y, w, h);

    /* Blit pitch must be dword-aligned.  Otherwise, the hardware appears to drop
     * the low bits.  Offsets must be naturally aligned.
     */
    if (src_pitch % 4 != 0 || src_offset % cpp != 0 ||
            dst_pitch % 4 != 0 || dst_offset % cpp != 0)
        return false;

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
            return false;
    }

    if (dst_y2 <= dst_y || dst_x2 <= dst_x) {
        return true;
    }

    assert(dst_x < dst_x2);
    assert(dst_y < dst_y2);

    BEGIN_BATCH(8);

    OUT_BATCH(CMD | (8 - 2));
    OUT_BATCH(BR13 | (uint16_t)dst_pitch);
    OUT_BATCH((dst_y << 16) | dst_x);
    OUT_BATCH((dst_y2 << 16) | dst_x2);
    OUT_RELOC_FENCED(dst_buffer,
            I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER,
            dst_offset);
    OUT_BATCH((src_y << 16) | src_x);
    OUT_BATCH((uint16_t)src_pitch);
    OUT_RELOC_FENCED(src_buffer,
            I915_GEM_DOMAIN_RENDER, 0,
            src_offset);

    ADVANCE_BATCH();

    intel_batchbuffer_emit_mi_flush(driver);

    return -1;
}

DriDriverOps* __dri_ex_driver_get(const char* driver_name)
{
    _MG_PRINTF("%s called with driver name: %s\n", __FUNCTION__, driver_name);

    if (strcmp(driver_name, "i915") == 0) {
        static DriDriverOps i915_driver = {
            .create_driver = i915_create_driver,
            .destroy_driver = i915_destroy_driver,
            .create_buffer = i915_create_buffer,
            .fetch_buffer = i915_fetch_buffer,
            .map_buffer = i915_map_buffer,
            .unmap_buffer = i915_unmap_buffer,
            .destroy_buffer = i915_destroy_buffer,
            .clear_buffer = i915_clear_buffer,
            .check_blit = i915_check_blit,
            .copy_blit = i915_copy_blit,
#if 0
            .begin_flush = i915_begin_flush,
            .end_flush = i915_end_flush,
#endif
        };

        return &i915_driver;
    }
    else {
        _MG_PRINTF("%s unknown DRM driver: %s\n", __FUNCTION__, driver_name);
    }

    return NULL;
}

#else /* HAVE_DRM_INTEL */

DriDriverOps* __dri_ex_driver_get(const char* driver_name)
{
    _WRN_PRINTF("This external DRM driver is a NULL implementation!");
    return NULL;
}

#endif /* !HAVE_DRM_INTEL */

#endif /* _MGGAL_DRI */

#endif /* __TARGET_EXTERNAL__ */
