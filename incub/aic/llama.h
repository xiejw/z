#ifndef AIC_LLAMA_H_
#define AIC_LLAMA_H_

#include <adt/types.h>
#include <adt/vec.h>

#include "tensor.h"
#include "vm.h"

struct llama_model {
        struct tensor *embedding;

        vec_t( struct tensor * ) tensors;
        struct vm *vm;
};

error_t model_new( struct ctx *ctx, const char *fname,
                   _OUT_ struct llama_model **model );
void    model_free( struct llama_model *p );
#endif  // AIC_LLAMA_H_
