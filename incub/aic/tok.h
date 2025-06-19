#ifndef AIC_TOK_H_
#define AIC_TOK_H_

#include <adt/types.h>
#include <adt/vec.h>

#include "ctx.h"

struct tokenizer;

/* Read tokenizer model file and create a tokenizer after that.
 *
 * Tokenizer model file should consist of lines of merge-able bytes with rank.
 */
ADT_NO_DISCARD error_t tok_new( struct ctx *ctx, const char *tok_model_name,
                                struct tokenizer **pp );
void                   tok_free( struct tokenizer *p );

/* Encodes the text and puts all tokens into ptokens.*/
ADT_NO_DISCARD error_t tok_encode( struct tokenizer *p, const char *text,
                                   vec_t( i64 ) * ptokens );

/* Encodes the text according to chat format. */
ADT_NO_DISCARD error_t tok_encode_chat( struct tokenizer *p, const char *text,
                                        vec_t( i64 ) * ptokens );

#endif /* AIC_TOK_H_ */
