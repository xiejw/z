#ifndef AIC_TENSOR_H_
#define AIC_TENSOR_H_

#include <adt/types.h>
#include <adt/vec.h>

#include "ctx.h"

#define AIC_TENSOR_MAX_RANK 4

struct shape {
        u32 rank;
        u32 dims[AIC_TENSOR_MAX_RANK];
        u64 ele_count;
};

struct tensor;

enum tensor_dtype {
        TSR_DTYPE_F32,
        TSR_DTYPE_I64,
};

/* Create a barebone tensor. Data buffer should be set later before use. */
struct tensor *tsr_new_without_data( enum tensor_dtype dtype, u32 rank,
                                     u32 *dims );
void           tsr_inc_ref( struct tensor *p );
void           tsr_dec_ref( struct tensor *p );
void           tsr_free_vec( vec_t( struct tensor * ) ptensors );

enum tensor_dtype   tsr_get_dtype( struct tensor * );
const struct shape *tsr_get_shape( struct tensor * );
f32                *tsr_get_f32_data( struct tensor * );
i64                *tsr_get_i64_data( struct tensor * );

/* Point the data to an unowned buffer.  */
void tsr_alias_data( struct tensor *tsr, void *data );
/* Allocates the data buffer and stores the pointer into pdata. */
void tsr_alloc_data( struct tensor *tsr, void **pdata );

/* Do a mmap from a file (fname) and map tensors to ptensors vector. */
ADT_NO_DISCARD error_t tsr_load_from_file( struct ctx *ctx, const char *fname,
                                           _OUT_ vec_t( struct tensor * ) *
                                               ptensors );

#endif  // AIC_TENSOR_H_
