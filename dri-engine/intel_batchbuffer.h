#ifndef INTEL_BATCHBUFFER_H
#define INTEL_BATCHBUFFER_H

#include "intel_reg.h"

/**
 * Number of bytes to reserve for commands necessary to complete a batch.
 *
 * This includes:
 * - MI_BATCHBUFFER_END (4 bytes)
 * - Optional MI_NOOP for ensuring the batch length is qword aligned (4 bytes)
 * - Any state emitted by vtbl->finish_batch():
 *   - Gen4-5 record ending occlusion query values (4 * 4 = 16 bytes)
 */
#define BATCH_RESERVED  24

#define BATCH_SIZE      256

struct intel_batchbuffer;

static int intel_batchbuffer_flush(struct _DriDriver *driver, unsigned int flags);

static inline uint32_t float_as_int(float f)
{
   union {
      float f;
      uint32_t d;
   } fi;

   fi.f = f;
   return fi.d;
}

/* Inline functions - might actually be better off with these
 * non-inlined.  Certainly better off switching all command packets to
 * be passed as structs rather than dwords, but that's a little bit of
 * work...
 */
static inline unsigned
intel_batchbuffer_space(struct _DriDriver *driver)
{
   return (driver->batch.bo->size - driver->batch.reserved_space)
      - driver->batch.used*4;
}

static inline void
intel_batchbuffer_emit_dword(struct _DriDriver *driver, GLuint dword)
{
#ifdef _DEBUG
   assert(intel_batchbuffer_space(driver) >= 4);
#endif
   driver->batch.map[driver->batch.used++] = dword;
}

static inline void
intel_batchbuffer_emit_float(struct _DriDriver *driver, float f)
{
   intel_batchbuffer_emit_dword(driver, float_as_int(f));
}

static inline void
intel_batchbuffer_require_space(struct _DriDriver *driver,
                                GLuint sz)
{
#ifdef _DEBUG
   assert(sz < driver->maxBatchSize - BATCH_RESERVED);
#endif
   if (intel_batchbuffer_space(driver) < sz)
      intel_batchbuffer_flush(driver, I915_EXEC_RENDER);
}

static inline void
intel_batchbuffer_begin(struct _DriDriver *driver, int n)
{
   intel_batchbuffer_require_space(driver, n * 4);

#ifdef _DEBUG
   driver->batch.emit = driver->batch.used;
   driver->batch.total = n;
#endif
}

static inline void
intel_batchbuffer_advance(struct _DriDriver *driver)
{
#ifdef _DEBUG
   struct intel_batchbuffer *batch = &driver->batch;
   unsigned int _n = batch->used - batch->emit;
   assert(batch->total != 0);
   if (_n != batch->total) {
      fprintf(stderr, "ADVANCE_BATCH: %d of %d dwords emitted\n",
          _n, batch->total);
      abort();
   }
   batch->total = 0;
#else
   (void) driver;
#endif
}

#endif
