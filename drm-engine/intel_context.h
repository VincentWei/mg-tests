///////////////////////////////////////////////////////////////////////////////
//
//                          IMPORTANT NOTICE
//
// The following open source license statement does not apply to any
// entity in the Exception List published by FMSoft.
//
// For more information, please visit:
//
// https://www.fmsoft.cn/exception-list
//
//////////////////////////////////////////////////////////////////////////////
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

//#define WITH_BUFFER_MANAGEMENT
#undef WITH_BUFFER_MANAGEMENT

#ifdef WITH_BUFFER_MANAGEMENT
#include "ply-hashtable.h"
#endif

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
struct _DrmDriver {
    int device_fd;
    drm_intel_bufmgr *manager;

    struct intel_batchbuffer batch;
    unsigned int maxBatchSize;

    int nr_buffers;
    int gen;
    uint32_t chip_id;
};

#endif
