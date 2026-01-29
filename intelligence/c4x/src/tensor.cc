#include "tensor.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ELE_DISPLAY 20 /* Max number of elements to display. */

namespace hermes {

void
show_tensor( Tensor *t, const char *prompt )
{
        if ( DISABLE_SHOW_TENSOR ) return;

        printf( "%s tensor data: ", prompt );
        for ( u32 i = 0; i < t->ele_total && i < MAX_ELE_DISPLAY; i++ ) {
                if ( i % 6 == 0 ) printf( "\n\t" );
                printf( "%g, ", t->data[i] );
        }
        printf( "\n" );
}

void
alloc_tensor( Tensor **dst, u32 dim, u32 *shape )
{
        Tensor *t = (Tensor *)malloc( sizeof( *t ) );
        assert( t != NULL );
        t->dim = dim;
        memcpy( t->shape, shape, sizeof( u32 ) * dim );
        *dst = t;

        u32 ele_total = 1;
        for ( u32 i = 0; i < dim; i++ ) {
                ele_total *= shape[i];
        }
        t->ele_total = ele_total;

        f32 *buf = (f32 *)malloc( sizeof( f32 ) * t->ele_total );
        assert( buf != NULL );
        t->data = buf;
}

void
dup_tensor( Tensor **dst, Tensor *src, int copy_data )
{
        Tensor *t = (Tensor *)malloc( sizeof( *t ) );
        assert( t != NULL );
        memcpy( t, src, sizeof( *t ) );
        *dst = t;

        f32 *buf = (f32 *)malloc( sizeof( f32 ) * t->ele_total );
        assert( buf != NULL );
        t->data = buf;

        if ( !copy_data ) return;

        memcpy( buf, src->data, sizeof( f32 ) * t->ele_total );
}

void
free_tensor( Tensor *p )
{
        if ( p == NULL ) return;
        free( p->data );
        free( p );
}

void
free_static_tensor_data( u32 tensor_cnt, Tensor *tensors )
{
        for ( u32 i = 0; i < tensor_cnt; i++ ) {
                free( tensors[i].data );
        }
}
}  // namespace hermes
