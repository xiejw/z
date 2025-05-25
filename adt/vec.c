#ifndef ADT_VEC_H_
#define ADT_VEC_H_

#include <assert.h>  // assert
#include <stdlib.h>  // free
#include <string.h>  // memcpy

// === --- Inject Common Types all ADTs Use ------------------------------------
//
// Assumes this code is same everywhere.
//
#ifndef ADT_TYPES_H_
#define ADT_TYPES_H_

// Primitive Types
#include <stdint.h>

typedef uint64_t u64;
typedef int64_t  i64;
typedef uint32_t u32;
typedef int32_t  i32;
typedef uint8_t  u8;
typedef float    f32;
typedef double   f64;

// Result and Error Codes
typedef int error_t;

#define OK        0
#define ERROR     -1
#define EMALLOC   -2
#define ENOTEXIST -3
#define ENOTIMPL  -4

// Function Parameter Annotation
#define _MUT_       // The field might be mutated if new address is allocated
#define _OUT_       // The field will be set with the output
#define _INOUT_     // The field will be passed in and then be set as output
#define _MOVED_IN_  // The ownership is moved into the method
#define _NULLABLE_  // The field is Nullable

#endif /* ADT_TYPES_H_ */

// === --- Design (Inspired by sds from redis) ---------------------------------
//
// +----+----+------
// |size|cap |buf
// +----+----+------
//           |
//  -2   -1   `-- ptr
//
// - Fast assessing:       Instead of a field lookup, `ptr` points to the buf
//                         head so ptr[x] can be used. With proper vecReserve,
//                         the address is safe.
//
// - Lazy initialization:  vecPushBack or vecReserve allocate the real memory.
//                         The vec must be initialized as NULL (vecNew).
//
// - Dynamic growth:       If the space is not enough, buf will be expanded to
//                         hold more elements in future.
//
// - Iteration:            for(size_t i = 0; i < vecSize(v); i++) { v[i] };
//
// - Fast modification:    Reserve a proper cap. Sets all values directly.
//                         Calls vecSetSize to corrects the size. Use with
//                         caution.
//
// === --- Ownership -----------------------------------------------------------
//
// 1. Container `vec` does not own any elements. So caller should free them.
// 2. Must call vecFree to release the memory, used by 'vec', on heap.
//
// === --- Contracts and Limitations -------------------------------------------
//
// 1. As the buf might be re-allocated (for growth), pass &vec for
//    modificaitons, e.g., vecReserve, vecPushBack, vecExtend.
// 2. When use vecSetSize, ensure the new values are initialized or old values
//    are freed properly.
//

// === --- APIs ----------------------------------------------------------------
//
// Conventions
// - vec is the plain pointer variable created by vecNew.
// - pvec is the address of vec.

#define vec_t( type ) type *

#define vecNew( )      NULL
#define vecFree( vec ) _VEC_FREE_IMPL( vec )

#define vecSize( vec )    ( ( vec ) ? ( (size_t *)( vec ) )[-2] : (size_t)0 )
#define vecCap( vec )     ( ( vec ) ? ( (size_t *)( vec ) )[-1] : (size_t)0 )
#define vecIsEmpty( vec ) ( vecSize( ( vec ) ) == 0 )

/* All following APIs return error_t to check errors. */
#define vecSetSize( vec, new_s )  _VEC_SET_SIZE_IMPL( vec, new_s )
#define vecReserve( pvec, count ) _VEC_RESERVE_IMPL( pvec, count )
#define vecPushBack( pvec, v )    _VEC_PUSH_BACK_IMPL( pvec, v )
#define vecExtend( pdst, src )    _VEC_EXTEND_IMPL( pdst, src )

/* Return last item. Must be non-empty. */
#define vecPopBack( vec ) _VEC_POP_BACK_IMPL( vec )

// === --- Private Macros and Prototypes ---------------------------------------
//

#define VEC_INIT_BUF_SIZE 16

static error_t _vecReserve( _MUT_ size_t **vec, size_t new_cap,
                            size_t unit_size ) __attribute__( ( unused ) );

static inline error_t _vecGrowSomeRoom( _MUT_ size_t **vec, size_t unit_size )
    __attribute__( ( unused ) );

static inline error_t _vecExtend( _MUT_ size_t **pdst, size_t dst_size,
                                  size_t unit_size, size_t *src,
                                  size_t src_size ) __attribute__( ( unused ) );

#define _VEC_FREE_IMPL( vec )                               \
        do {                                                \
                if ( ( vec ) != NULL ) {                    \
                        free( &( (size_t *)( vec ) )[-2] ); \
                }                                           \
        } while ( 0 )

#define _VEC_SET_SIZE_IMPL( vec, new_s ) \
        ( ( vec ) ? ( ( (size_t *)vec )[-2] = ( new_s ), OK ) : ENOTEXIST )

#define _VEC_RESERVE_IMPL( pvec, count )         \
        _vecReserve( (size_t **)( pvec ), count, \
                     /*unit_size=*/sizeof( **( pvec ) ) )

#define _VEC_PUSH_BACK_IMPL( pvec, v )                                  \
        ( _vecGrowSomeRoom( (size_t **)( pvec ),                        \
                            /*unit_size=*/sizeof( **( pvec ) ) ) ||     \
          ( ( ( *( pvec ) )[( (size_t *)( *( pvec ) ) )[-2]] = ( v ) ), \
            ( (size_t *)( *( pvec ) ) )[-2]++, OK ) )

#define _VEC_EXTEND_IMPL( pdst, src )                                      \
        _vecExtend( (size_t **)( pdst ), vecSize( *( pdst ) ),             \
                    /*unit_size=*/sizeof( **( pdst ) ), (size_t *)( src ), \
                    vecSize( ( src ) ) )

#define _VEC_POP_BACK_IMPL( vec )    \
        ( assert( ( vec ) != NULL ), \
          *( ( vec ) + --( ( (size_t *)( vec ) )[-2] ) ) )

error_t
_vecGrowSomeRoom( _MUT_ size_t **vec, size_t unit_size )
{
        if ( !*vec ) {
                return _vecReserve( vec, VEC_INIT_BUF_SIZE, unit_size );
        }

        const size_t cap  = ( *vec )[-1];
        const size_t size = ( *vec )[-2];
        assert( size <= cap );
        if ( cap != size ) return OK;
        return _vecReserve( vec, 2 * cap, unit_size );
}

error_t
_vecExtend( _MUT_ size_t **pdst, size_t dst_size, size_t unit_size, size_t *src,
            size_t src_size )
{
        const size_t new_size = dst_size + src_size;
        error_t      err      = _vecReserve( pdst, new_size, unit_size );
        if ( err ) return err;
        memcpy( ( (char *)( *pdst ) ) + unit_size * dst_size, src,
                unit_size * src_size );
        ( *pdst )[-2] = new_size;
        return OK;
}

error_t
_vecReserve( size_t **vec, size_t new_cap, size_t unit_size )
{
        const size_t head_size = 2;

        const size_t new_s = (new_cap)*unit_size + head_size * sizeof( size_t );

        if ( *vec ) {
                size_t *new_p =
                    realloc( &( *vec )[-(ssize_t)head_size], new_s );
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
#endif /* ADT_VEC_H_ */

/* === --- Test Code -------------------------------------------------------- */

#ifdef ADT_TEST_H_
#include <stdio.h>

int
main( void )
{
        vec_t( char ) v = vecNew( );
        assert( vecSize( v ) == 0 );
        assert( vecCap( v ) == 0 );

        vecPushBack( &v, 'a' );
        assert( vecSize( v ) == 1 );
        vecPushBack( &v, 'b' );
        assert( vecSize( v ) == 2 );
        assert( vecCap( v ) == VEC_INIT_BUF_SIZE );

        vecReserve( &v, 1024 );
        assert( vecCap( v ) >= 1024 );

        for ( size_t i = 0; i < vecSize( v ); i++ ) {
                assert( i == 0 ? v[i] == 'a' : v[i] == 'b' );
        }

        assert( vecPopBack( v ) == 'b' );
        assert( vecPopBack( v ) == 'a' );
        assert( vecSize( v ) == 0 );
        assert( vecIsEmpty( v ) );
        vecFree( v );

        printf( "Test passed.\n" );
        return 0;
}

#endif /* ADT_TEST_H_ */
