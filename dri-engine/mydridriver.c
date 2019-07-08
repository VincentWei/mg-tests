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

#include "ply-hashtable.h"

/* the driver data struct */
struct _DriDriver {
    int device_fd;
    drm_intel_bufmgr *manager;

    ply_hashtable_t *buffers;
};

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
    _DBG_PRINTF ("%s: destroying intel buffer manager\n",
            __FUNCTION__);

    ply_hashtable_free (driver->buffers);
    drm_intel_bufmgr_destroy (driver->manager);
    free (driver);
}

typedef struct _drm_buffer drm_buffer_t;

struct _drm_buffer
{
    drm_intel_bo *object;
    uint32_t id;
    unsigned long width;
    unsigned long height;
    unsigned long pitch;

    uint32_t added_fb : 1;
};

#define ROUND_TO_MULTIPLE(n, m) (((n) + (((m) - 1))) & ~((m) - 1))

static drm_buffer_t * drm_buffer_new (DriDriver *driver,
        drm_intel_bo *buffer_object,
        uint32_t id,
        unsigned long width,
        unsigned long height,
        unsigned long pitch)
{
    drm_buffer_t *buffer;

    buffer = calloc (1, sizeof (drm_buffer_t));
    buffer->object = buffer_object;
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

static drm_buffer_t* drm_buffer_new_from_id (DriDriver *driver,
        uint32_t buffer_id)
{
    drm_buffer_t *buffer;
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

static drm_buffer_t *get_buffer_from_id (DriDriver *driver,
        uint32_t buffer_id)
{
    static drm_buffer_t *buffer;

    buffer = ply_hashtable_lookup (driver->buffers,
            (void *) (uintptr_t) buffer_id);

    return buffer;
}

static uint32_t i915_create_buffer (DriDriver *driver,
            int depth, int bpp,
            unsigned int width, unsigned int height,
            unsigned int *pitch)
{
    drm_intel_bo *buffer_object;
    drm_buffer_t *buffer;
    uint32_t buffer_id;

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
    buffer->added_fb = TRUE;
    ply_hashtable_insert (driver->buffers,
            (void *) (uintptr_t) buffer_id,
            buffer);

    return buffer_id;
}

static BOOL i915_fetch_buffer (DriDriver *driver,
            uint32_t  buffer_id,
            unsigned int *width, unsigned int *height,
            unsigned int *pitch)
{
    drm_buffer_t *buffer;

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
    drm_buffer_t *buffer;

    buffer = get_buffer_from_id (driver, buffer_id);

    assert (buffer != NULL);
    drm_intel_gem_bo_map_gtt (buffer->object);

    return buffer->object->virtual;
}

static void i915_unmap_buffer (DriDriver *driver,
            uint32_t buffer_id)
{
    drm_buffer_t *buffer;

    buffer = get_buffer_from_id (driver, buffer_id);
    assert (buffer != NULL);

    drm_intel_gem_bo_unmap_gtt (buffer->object);
}

#if 0
static uint8_t * i915_begin_flush (DriDriver *driver,
            uint32_t buffer_id)
{
    drm_buffer_t *buffer;

    buffer = get_buffer_from_id (driver, buffer_id);
    assert (buffer != NULL);

    return buffer->object->virtual;
}

static void i915_end_flush (DriDriver *driver,
            uint32_t buffer_id)
{
    drm_buffer_t *buffer;

    buffer = get_buffer_from_id (driver, buffer_id);
    assert (buffer != NULL);
}
#endif

static void i915_destroy_buffer (DriDriver *driver,
            uint32_t buffer_id)
{
    drm_buffer_t *buffer;

    buffer = get_buffer_from_id (driver, buffer_id);

    assert (buffer != NULL);

    if (buffer->added_fb)
        drmModeRmFB (driver->device_fd, buffer->id);

    drm_intel_bo_unreference (buffer->object);

    ply_hashtable_remove (driver->buffers,
            (void *) (uintptr_t) buffer_id);

    free (buffer);
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
