#ifndef MLM_TOK_H_
#define MLM_TOK_H_

#include <adt/types.h>

struct tokenizer;

struct tokenizer *tok_new( void );
void              tok_free( struct tokenizer *p );

/* Read tokenizer model file and create a tokenizer after that.
 *
 * Tokenizer model file should consist of lines of merge-able bytes with rank.
 */
error_t tok_load( struct tokenizer *p, const char *model_fname );

void tok_encode( struct tokenizer *p, const char *text );

#endif /* MLM_TOK_H_ */
