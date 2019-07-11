/**************************************************************************
 * 
 * Copyright 2003 VMware, Inc.
 * Copyright 2019 FMSoft (http://www.fmsoft.cn).
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#ifndef INTELCONTEXT_INC
#define INTELCONTEXT_INC

#include <stdbool.h>
#include <string.h>

#include <drm.h>
#include <i915_drm.h>
#include <libdrm/intel_bufmgr.h>

#include "ply-hashtable.h"

#define DV_PF_555  (1<<8)
#define DV_PF_565  (2<<8)
#define DV_PF_8888 (3<<8)
#define DV_PF_4444 (8<<8)
#define DV_PF_1555 (9<<8)

#define INTEL_WRITE_PART  0x1
#define INTEL_WRITE_FULL  0x2
#define INTEL_READ        0x4

#ifndef likely
#ifdef __GNUC__
#define likely(expr) (__builtin_expect(expr, 1))
#define unlikely(expr) (__builtin_expect(expr, 0))
#else
#define likely(expr) (expr)
#define unlikely(expr) (expr)
#endif
#endif

struct intel_batchbuffer {
    /** Current batchbuffer being queued up. */
    drm_intel_bo *bo;

#ifdef _DEBUG
    uint16_t emit, total;
#endif
    uint16_t used, reserved_space;
    uint32_t *map;
    uint32_t *cpu_map;
#define BATCH_SZ (8192*sizeof(uint32_t))
};

#define GLuint unsigned int

/* the driver data struct */
struct _DriDriver {
    int device_fd;
    drm_intel_bufmgr *manager;

    struct intel_batchbuffer batch;
    unsigned int maxBatchSize;

    ply_hashtable_t *buffers;
    int nr_buffers;

#if 0
    drm_intel_bo *first_post_swapbuffers_batch;
    uint32_t no_batch_wrap:1;

    struct {
        drm_intel_bo *bo;
        GLuint offset;
        uint32_t buffer_len;
        uint32_t buffer_offset;
        char buffer[4096];
    } upload;

    // i915-specific context
    struct i915_hw_state
    {
       GLuint active;               /* I915_UPLOAD_* */
       GLuint emitted;              /* I915_UPLOAD_* */
    } state;

    drm_intel_bo *current_vb_bo;
    unsigned int current_vertex_size;

    uint32_t last_draw_offset;
    GLuint last_sampler;
#endif
};

#endif
