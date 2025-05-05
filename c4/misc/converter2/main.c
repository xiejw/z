#include <assert.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BIN_DATA_FILE    "tensor_data.bin"
#define MAX_DIM_LIMIT    5
#define MAX_TENSOR_LIMIT 128
#define MAX_ELE_DISPLAY  20

#define PANIC( msg )                 \
        do {                         \
                printf( "panic\n" ); \
                printf( msg );       \
                exit( -1 );          \
        } while ( 0 )

typedef float    f32;
typedef uint32_t u32;

typedef struct {
        u32  dim;
        u32  shape[MAX_DIM_LIMIT];
        u32  ele_total;
        f32 *data;
} Tensor;

void
read_tensor_cnt( int fd, u32 *cnt )
{
        int c = read( fd, cnt, 4 );
        if ( c < 0 ) PANIC( "failed to read bytes for a u32" );
        assert( *cnt <= MAX_TENSOR_LIMIT );
        printf( "tensor count %u\n", *cnt );
}

void
read_shape( int fd, Tensor *t )
{
        u32 dim;
        int c = read( fd, &dim, 4 );
        if ( c < 0 ) PANIC( "failed to read bytes for a u32" );
        assert( dim <= MAX_DIM_LIMIT );
        printf( "dim %u\n", dim );
        t->dim = dim;

        u32 ele_total = 1;
        u32 ele;
        printf( "shape <" );
        for ( u32 i = 0; i < dim; i++ ) {
                int c = read( fd, &ele, 4 );
                if ( c < 0 ) PANIC( "failed to read bytes for a u32" );

                printf( "%u, ", ele );
                t->shape[i] = ele;
                ele_total *= ele;
        }
        t->ele_total = ele_total;
        printf( ">\n" );
        printf( "ele_total: %u\n", ele_total );
}

void
read_tensor( int fd, Tensor *t )
{
        size_t byte_count = sizeof( f32 ) * t->ele_total;
        t->data           = malloc( byte_count );
        assert( t->data != NULL );

        f32 f;
        printf( "tensor data: " );
        for ( int i = 0; i < t->ele_total; i++ ) {
                int c = read( fd, &f, 4 );
                if ( c < 0 ) PANIC( "failed to read bytes for a f32" );

                t->data[i] = f;

                if ( i < MAX_ELE_DISPLAY ) printf( "%g, ", f );
                if ( i == MAX_ELE_DISPLAY || ( t->ele_total < MAX_ELE_DISPLAY &&
                                               i == t->ele_total - 1 ) )
                        printf( "\n" );
        }
}

void
read_tensor_data( int fd, u32 *tensor_cnt, Tensor *tensors )
{
        read_tensor_cnt( fd, tensor_cnt );

        /* Pass 1: Read shapes */
        for ( u32 i = 0; i < *tensor_cnt; i++ ) {
                read_shape( fd, &tensors[i] );
        }
        /* Pass 2: Read tensor data */
        for ( u32 i = 0; i < *tensor_cnt; i++ ) {
                read_tensor( fd, &tensors[i] );
        }
}

void
free_tensor_data( u32 tensor_cnt, Tensor *tensors )
{
        for ( u32 i = 0; i < tensor_cnt; i++ ) {
                free( tensors[i].data );
        }
}

int
main( int argc, char **argv )
{
        int fd = open( BIN_DATA_FILE, O_RDONLY );
        if ( fd == -1 ) PANIC( "failed to open tensor data file" );

        u32    tensor_cnt;
        Tensor tensors[MAX_TENSOR_LIMIT];
        read_tensor_data( fd, &tensor_cnt, tensors );

        free_tensor_data( tensor_cnt, tensors );
        close( fd );
}
