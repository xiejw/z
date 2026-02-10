#include <zion/zion.h>

/* === --- Test Code -------------------------------------------------------- */

#include <stdio.h>

int
main( void )
{
        vec_t( char ) v = vec_new( );
        assert( vec_size( v ) == 0 );
        assert( vec_cap( v ) == 0 );

        vec_push( &v, 'a' );
        assert( vec_size( v ) == 1 );
        vec_push( &v, 'b' );
        assert( vec_size( v ) == 2 );
        assert( vec_cap( v ) == VEC_INIT_BUF_SIZE );

        vec_reserve( &v, 1024 );
        assert( vec_cap( v ) >= 1024 );

        for ( size_t i = 0; i < vec_size( v ); i++ ) {
                assert( i == 0 ? v[i] == 'a' : v[i] == 'b' );
        }

        assert( vec_pop( v ) == 'b' );
        assert( vec_pop( v ) == 'a' );
        assert( vec_size( v ) == 0 );
        assert( vec_is_empty( v ) );
        vec_free( v );

        printf( "Test passed.\n" );
        return 0;
}
