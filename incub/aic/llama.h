#ifndef AIC_LLAMA_H_
#define AIC_LLAMA_H_

#include <adt/types.h>
#include <adt/vec.h>

#include "tensor.h"
#include "vm.h"

struct llama_model;

/* Creates a new llama model, in model by loading model weights from file fname.
 */
ADT_NO_DISCARD error_t model_new( struct ctx *ctx, const char *fname,
                                  _OUT_ struct llama_model **model );
void                   model_free( struct llama_model *p );

ADT_NO_DISCARD error_t model_run( struct llama_model *model,
                                  vec_t( i64 ) tokens );
#endif  // AIC_LLAMA_H_
