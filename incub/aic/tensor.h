#ifndef AIC_TENSOR_H_
#define AIC_TENSOR_H_

#include <adt/types.h>
#include <adt/vec.h>

#include "ctx.h"

#define AIC_TENSOR_MAX_RANK 4

struct tensor;

/* Create a barebone tensor. dtype, alias and data points should be set. */
struct tensor *tsr_new_without_data( u32 rank, u32 *dims );
void           tsr_inc_ref( struct tensor *p );
void           tsr_dec_ref( struct tensor *p );
void           tsr_free_vec( vec_t( struct tensor * ) ptensors );

/* Change dtype of tensor. Must do before setting data */
#define TSR_DTYPE_F32 0
#define TSR_DTYPE_I64 1
void tsr_set_dtype( struct tensor *tsr, char dtype );

/* Point the data to an unowned buffer. Must set dtype first.  */
void tsr_alias_data( struct tensor *tsr, void *data );

/* Do a mmap from a file (fname) and map tensors to ptensors vector. */
ADT_NO_DISCARD error_t tsr_load_from_file( struct ctx *ctx, const char *fname,
                                           _OUT_ vec_t( struct tensor * ) *
                                               ptensors );

#endif  // AIC_TENSOR_H_
