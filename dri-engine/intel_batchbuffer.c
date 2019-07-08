/**************************************************************************
 *
 * Copyright 2006 VMware, Inc.
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
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "intel_context.h"
#include "intel_batchbuffer.h"
#include "intel_reg.h"

static void
intel_batchbuffer_reset(struct _DriDriver *driver);

    void
intel_batchbuffer_init(struct _DriDriver *driver)
{
    intel_batchbuffer_reset(driver);

    driver->batch.cpu_map = malloc(driver->maxBatchSize);
    driver->batch.map = driver->batch.cpu_map;
}

static void
intel_batchbuffer_reset(struct _DriDriver *driver)
{
    if (driver->batch.last_bo != NULL) {
        drm_intel_bo_unreference(driver->batch.last_bo);
        driver->batch.last_bo = NULL;
    }
    driver->batch.last_bo = driver->batch.bo;

    driver->batch.bo = drm_intel_bo_alloc(driver->manager, "batchbuffer",
            driver->maxBatchSize, 4096);

    driver->batch.reserved_space = BATCH_RESERVED;
    driver->batch.used = 0;
}

void
intel_batchbuffer_free(struct _DriDriver *driver)
{
    free(driver->batch.cpu_map);
    drm_intel_bo_unreference(driver->batch.last_bo);
    drm_intel_bo_unreference(driver->batch.bo);
}

#if 0
static void
do_batch_dump(struct _DriDriver *driver)
{
    struct drm_intel_decode *decode;
    struct intel_batchbuffer *batch = &driver->batch;
    int ret;

    decode = drm_intel_decode_context_alloc(driver->intelScreen->deviceID);
    if (!decode)
        return;

    ret = drm_intel_bo_map(batch->bo, false);
    if (ret == 0) {
        drm_intel_decode_set_batch_pointer(decode,
                batch->bo->virtual,
                batch->bo->offset,
                batch->used);
    } else {
        fprintf(stderr,
                "WARNING: failed to map batchbuffer (%s), "
                "dumping uploaded data instead.\n", strerror(ret));

        drm_intel_decode_set_batch_pointer(decode,
                batch->map,
                batch->bo->offset,
                batch->used);
    }

    drm_intel_decode(decode);

    drm_intel_decode_context_free(decode);

    if (ret == 0) {
        drm_intel_bo_unmap(batch->bo);

        if (driver->vtbl.debug_batch != NULL)
            driver->vtbl.debug_batch(driver);
    }
}
#endif

static void i915_new_batch(struct _DriDriver *driver)
{
    /* Mark all state as needing to be emitted when starting a new batchbuffer.
     * Using hardware contexts would be an alternative, but they have some
     * difficulties associated with them (physical address requirements).
     */
    driver->state.emitted = 0;
    driver->last_draw_offset = 0;
    driver->last_sampler = 0;

    driver->current_vb_bo = NULL;
    driver->current_vertex_size = 0;
}

/* TODO: Push this whole function into manager.
 */
static int
do_flush_locked(struct _DriDriver *driver)
{
    struct intel_batchbuffer *batch = &driver->batch;
    int ret = 0;

    ret = drm_intel_bo_subdata(batch->bo, 0, 4*batch->used, batch->map);

#if 0
    if (!driver->intelScreen->no_hw) {
        if (ret == 0) {
            if (unlikely(INTEL_DEBUG & DEBUG_AUB) && driver->vtbl.annotate_aub)
                driver->vtbl.annotate_aub(driver);
            ret = drm_intel_bo_mrb_exec(batch->bo, 4 * batch->used, NULL, 0, 0,
                    I915_EXEC_RENDER);
        }
    }
#else
    ret = drm_intel_bo_mrb_exec(batch->bo, 4 * batch->used, NULL, 0, 0,
            I915_EXEC_RENDER);
#endif

#if 0
    if (unlikely(INTEL_DEBUG & DEBUG_BATCH))
        do_batch_dump(driver);
#endif

    if (ret != 0) {
        fprintf(stderr, "intel_do_flush_locked failed: %s\n", strerror(-ret));
        exit(1);
    }

    i915_new_batch(driver);

    return ret;
}

static void intel_upload_finish(struct _DriDriver *driver)
{
    if (!driver->upload.bo)
        return;

    if (driver->upload.buffer_len) {
        drm_intel_bo_subdata(driver->upload.bo,
                driver->upload.buffer_offset,
                driver->upload.buffer_len,
                driver->upload.buffer);
        driver->upload.buffer_len = 0;
    }

    drm_intel_bo_unreference(driver->upload.bo);
    driver->upload.bo = NULL;
}

int
_intel_batchbuffer_flush(struct _DriDriver *driver,
        const char *file, int line)
{
    int ret;

    if (driver->batch.used == 0)
        return 0;

    if (driver->first_post_swapbuffers_batch == NULL) {
        driver->first_post_swapbuffers_batch = driver->batch.bo;
        drm_intel_bo_reference(driver->first_post_swapbuffers_batch);
    }

#if 0
    if (unlikely(INTEL_DEBUG & DEBUG_BATCH))
        fprintf(stderr, "%s:%d: Batchbuffer flush with %db used\n", file, line,
                4*driver->batch.used);
#else
#endif

    driver->batch.reserved_space = 0;

#if 0
    if (driver->vtbl.finish_batch)
        driver->vtbl.finish_batch(driver);
#endif

    /* Mark the end of the buffer. */
    intel_batchbuffer_emit_dword(driver, MI_BATCH_BUFFER_END);
    if (driver->batch.used & 1) {
        /* Round batchbuffer usage to 2 DWORDs. */
        intel_batchbuffer_emit_dword(driver, MI_NOOP);
    }

    intel_upload_finish(driver);

    /* Check that we didn't just wrap our batchbuffer at a bad time. */
    assert(!driver->no_batch_wrap);

    ret = do_flush_locked(driver);

#if 0
    if (unlikely(INTEL_DEBUG & DEBUG_SYNC)) {
        fprintf(stderr, "waiting for idle\n");
        drm_intel_bo_wait_rendering(driver->batch.bo);
    }
#endif

    /* Reset the buffer:
     */
    intel_batchbuffer_reset(driver);

    return ret;
}


/*  This is the only way buffers get added to the validate list.
 */
bool
intel_batchbuffer_emit_reloc(struct _DriDriver *driver,
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

bool
intel_batchbuffer_emit_reloc_fenced(struct _DriDriver *driver,
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

void
intel_batchbuffer_data(struct _DriDriver *driver,
        const void *data, GLuint bytes)
{
    assert((bytes & 3) == 0);
    intel_batchbuffer_require_space(driver, bytes);
    memcpy(driver->batch.map + driver->batch.used, data, bytes);
    driver->batch.used += bytes >> 2;
}

/* Emit a pipelined flush to either flush render and texture cache for
 * reading from a FBO-drawn texture, or flush so that frontbuffer
 * render appears on the screen in DRI1.
 *
 * This is also used for the always_flush_cache driconf debug option.
 */
void
intel_batchbuffer_emit_mi_flush(struct _DriDriver *driver)
{
    BEGIN_BATCH(1);
    OUT_BATCH(MI_FLUSH);
    ADVANCE_BATCH();
}
