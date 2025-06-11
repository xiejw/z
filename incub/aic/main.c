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
        // err = tok_encode( p, "What is the answer of 1+1?", &tokens );
        err = tok_encode(
            p, "What is the answer of the super unbelievably simple math 1+1?",
            &tokens );
        PANIC_IF_ERR( err, ctx );

        printf( "tokens " );
        for ( size_t i = 0; i < vec_size( tokens ); i++ ) {
                printf( "%zu, ", tokens[i] );
        }
        printf( "\n" );

        vec_free( tokens );
        tok_free( p );
        ctx_free( ctx );
}
