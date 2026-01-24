#include <zion/zion.h>

/* === --- Test Code -------------------------------------------------------- */

#include <stdio.h>
int
main( void )
{
        sds_t s = sds_new( "hello " );
        assert( 0 == strcmp( "hello ", s ) );

        sds_cat_printf( &s, "%s", "world" );
        assert( 0 == strcmp( "hello world", s ) );

        sds_free( s );

        printf( "Test passed.\n" );
        return 0;
}
