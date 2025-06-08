#include "tok.h"

#include <stdio.h>
#include <stdlib.h>

/* === Configuration -------------------------------------------------------- */
#ifndef TOK_FILE
#error TOK_FILE should be passed via "-DTOK_FILE"
#endif

#define PANIC( )                     \
        do {                         \
                printf( "panic\n" ); \
                exit( -1 );          \
        } while ( 0 )

#define PANIC_IF_ERR( err ) _PANIC_IF_ERR( err, __FILE__, __LINE__ )

#define _PANIC_IF_ERR( err, file, line )                            \
        do {                                                        \
                if ( ( err ) != OK ) {                              \
                        printf( "file %s, line %d\n", file, line ); \
                        PANIC( );                                   \
                }                                                   \
        } while ( 0 )

/* === Main ----------------------------------------------------------------- */

int
main( void )
{
        struct tokenizer *p;
        error_t           err = tok_new( &p, TOK_FILE );
        PANIC_IF_ERR( err );

        vec_t( int ) tokens = vec_new( );
        tok_encode( p, "hello world   ", &tokens );

        vec_free( tokens );
        tok_free( p );
}
