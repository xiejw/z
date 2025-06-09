#include "ctx.h"
#include "tok.h"

#include <stdio.h>
#include <stdlib.h>

/* === Configuration -------------------------------------------------------- */
#ifndef TOK_FILE
#error TOK_FILE should be passed via "-DTOK_FILE"
#endif

#define PANIC_IF_ERR( err, ctx ) _PANIC_IF_ERR( err, ctx, __FILE__, __LINE__ )

#define _PANIC_IF_ERR( err, ctx, file, line )      \
        do {                                       \
                if ( ( err ) != OK ) {             \
                        LOG_ERROR( ctx, "panic" ); \
                        DUMP_ERROR_NOTE( ctx );    \
                        exit( -1 );                \
                }                                  \
        } while ( 0 )

/* === Main ----------------------------------------------------------------- */

int
main( void )
{
        struct ctx       *ctx = ctx_new( );
        struct tokenizer *p;
        error_t           err = tok_new( ctx, TOK_FILE, &p );
        PANIC_IF_ERR( err, ctx );

        vec_t( size_t ) tokens = vec_new( );
        err                    = tok_encode( p, "hello world   ", &tokens );
        PANIC_IF_ERR( err, ctx );

        vec_free( tokens );
        tok_free( p );
        ctx_free( ctx );
}
