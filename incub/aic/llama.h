#ifndef AIC_LLAMA_H_
#define AIC_LLAMA_H_

#include <adt/types.h>
#include <adt/vec.h>

#include "tensor.h"
#include "vm.h"

struct weight_info {
        const char    *name;   /* Alias */
        struct tensor *weight; /* Alias */
};

struct llama_model {
        struct ctx *ctx;                  /* Now owned. */
        struct vm  *vm;                   /* Owned. */
        vec_t( struct tensor * ) tensors; /* Owned */

        struct weight_info embedding;
};

error_t model_new( struct ctx *ctx, const char *fname,
                   _OUT_ struct llama_model **model );
void    model_free( struct llama_model *p );

error_t model_run( struct llama_model *model, vec_t( i64 ) tokens );
#endif  // AIC_LLAMA_H_
