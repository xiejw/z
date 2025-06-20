#ifndef AIC_LLAMA_H_
#define AIC_LLAMA_H_

#include <adt/types.h>
#include <adt/vec.h>

#include "tensor.h"
#include "vm.h"

struct llama_model;

/* Creates a new llama model, stored in 'pmodel'. The model weights are loaded
 * from the weights file, specified by the fname.
 */
ADT_NO_DISCARD error_t llama_model_new( struct ctx *ctx, const char *fname,
                                        _OUT_ struct llama_model **pmodel );
void                   llama_model_free( struct llama_model *p );

ADT_NO_DISCARD error_t llama_model_run( struct llama_model *model,
                                        const vec_t( i64 ) tokens );
#endif  // AIC_LLAMA_H_
