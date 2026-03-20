/* par.h — forge_par_map: pthreads-based parallel map */

#ifndef PAR_H
#define PAR_H

#include <stddef.h>

#include "error.h"

// === --- forge_par_map ------------------------------------------------------
// ===
//

/* Parallel map over n items using pthreads.
 *
 * Calls fn(i, slot_out, ctx) for each i in [0, n), where
 *   slot_out = (char *)dst_out + i * dst_stride.
 * fn must be thread-safe; ctx is read-only shared context.
 * n_threads == 0: auto-detect via sysconf(_SC_NPROCESSORS_ONLN).
 * Returns 0 on success, 1 on error (written into stk). */
int forge_par_map( size_t n,
                   void ( *fn )( size_t i, void *slot_out, const void *ctx ),
                   void *dst_out, size_t dst_stride,
                   const void *ctx,
                   size_t n_threads,
                   struct err_stack *stk );

#endif /* PAR_H */
