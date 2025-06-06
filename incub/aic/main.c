#include "tok.h"

/* === Configuration -------------------------------------------------------- */
#ifndef TOK_FILE
#error TOK_FILE should be passed via "-DTOK_FILE"
#endif

/* === Main ----------------------------------------------------------------- */

int
main( void )
{
        struct tokenizer *p = tok_new( );
        tok_load( p, TOK_FILE );
        tok_encode( p, "hello world   " );
        tok_free( p );
}
