#ifndef MLM_TOK_H_
#define MLM_TOK_H_

#include <adt/types.h>

struct tokenizer;

/* Read tokenizer model file and create a tokenizer after that.
 *
 * Tokenizer model file should consist of lines of merge-able bytes with rank.
 */
error_t tok_new( struct tokenizer **pp, const char *tok_model_name );
void    tok_free( struct tokenizer *p );

void tok_encode( struct tokenizer *p, const char *text );

#endif /* MLM_TOK_H_ */
