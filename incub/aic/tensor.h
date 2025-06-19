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

struct tensor {
        struct shape sp;
        char         dtype;   /* 0 f32 1 i64 */
        char         alias;   /* 0 owned 1 alias */
        size_t       ref_cnt; /* reference count. */
        union {
                f32 *f;
                i64 *i;
        };
};

/* Create a barebone tensor. dtype, alias and data points should be set. */
struct tensor *tsr_new_without_data( u32 rank, u32 *dims );
void           tsr_inc_ref( struct tensor *p );
void           tsr_dec_ref( struct tensor *p );
void           tsr_free_vec( vec_t( struct tensor * ) ptensors );

/* Do a mmap from a file (fname) and map tensors to ptensors vector. */
ADT_NO_DISCARD error_t tsr_load_from_file( struct ctx *ctx, const char *fname,
                                           _OUT_ vec_t( struct tensor * ) *
                                               ptensors );

#endif  // AIC_TENSOR_H_
