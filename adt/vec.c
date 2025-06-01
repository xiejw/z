#include <adt/vec.h>

/* === --- Implementation --------------------------------------------------- */

error_t
_vec_reserve( size_t **vec, size_t new_cap, size_t unit_size )
{
        const size_t head_size = 2;

        const size_t new_s = (new_cap)*unit_size + head_size * sizeof( size_t );

        if ( *vec ) {
                size_t *new_p =
                    realloc( &( *vec )[-(ptrdiff_t)head_size], new_s );
                if ( new_p == NULL ) return EMALLOC;
                new_p[head_size - 1] = new_cap;  // other head fields unchanged.
                *vec                 = (void *)( new_p + head_size );
        } else {
                size_t *p = malloc( new_s );
                if ( p == NULL ) return EMALLOC;
                p[head_size - 2] = 0;
                p[head_size - 1] = new_cap;
                *vec             = (void *)( p + head_size );
        }
        return OK;
}

/* === --- Test Code -------------------------------------------------------- */

#ifdef ADT_TEST_H_
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

#endif /* ADT_TEST_H_ */
