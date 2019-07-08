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
#define BATCH_RESERVED 24

struct intel_batchbuffer;

void intel_batchbuffer_init(struct _DriDriver *driver);
void intel_batchbuffer_free(struct _DriDriver *driver);

int _intel_batchbuffer_flush(struct _DriDriver *driver,
                 const char *file, int line);

#define intel_batchbuffer_flush(driver) \
    _intel_batchbuffer_flush(driver, __FILE__, __LINE__)



/* Unlike bmBufferData, this currently requires the buffer be mapped.
 * Consider it a convenience function wrapping multiple
 * intel_buffer_dword() calls.
 */
void intel_batchbuffer_data(struct _DriDriver *driver,
                        const void *data, GLuint bytes);

bool intel_batchbuffer_emit_reloc(struct _DriDriver *driver,
                        drm_intel_bo *buffer,
                        uint32_t read_domains,
                        uint32_t write_domain,
                        uint32_t offset);
bool intel_batchbuffer_emit_reloc_fenced(struct _DriDriver *driver,
                        drm_intel_bo *buffer,
                        uint32_t read_domains,
                        uint32_t write_domain,
                        uint32_t offset);
void intel_batchbuffer_emit_mi_flush(struct _DriDriver *driver);

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
#ifdef DEBUG
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
#ifdef DEBUG
   assert(sz < driver->maxBatchSize - BATCH_RESERVED);
#endif
   if (intel_batchbuffer_space(driver) < sz)
      intel_batchbuffer_flush(driver);
}

static inline void
intel_batchbuffer_begin(struct _DriDriver *driver, int n)
{
   intel_batchbuffer_require_space(driver, n * 4);

   driver->batch.emit = driver->batch.used;
#ifdef DEBUG
   driver->batch.total = n;
#endif
}

static inline void
intel_batchbuffer_advance(struct _DriDriver *driver)
{
#ifdef DEBUG
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

/* Here are the crusty old macros, to be removed:
 */
#define BATCH_LOCALS

#define BEGIN_BATCH(n) intel_batchbuffer_begin(driver, n)
#define OUT_BATCH(d) intel_batchbuffer_emit_dword(driver, d)
#define OUT_BATCH_F(f) intel_batchbuffer_emit_float(driver,f)
#define OUT_RELOC(buf, read_domains, write_domain, delta) do {        \
   intel_batchbuffer_emit_reloc(driver, buf,            \
                read_domains, write_domain, delta);    \
} while (0)
#define OUT_RELOC_FENCED(buf, read_domains, write_domain, delta) do {    \
   intel_batchbuffer_emit_reloc_fenced(driver, buf,        \
                       read_domains, write_domain, delta); \
} while (0)

#define ADVANCE_BATCH() intel_batchbuffer_advance(driver);
#define CACHED_BATCH() intel_batchbuffer_cached_advance(driver);

#endif
