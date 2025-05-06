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
typedef int32_t  i32;

typedef struct {
        u32  dim;
        u32  shape[MAX_DIM_LIMIT];
        u32  ele_total;
        f32 *data;
} Tensor;

/* === Utils to display tensor ---------------------------------------------- */
void
show_tensor( Tensor *t, const char *prompt )
{
        printf( "%s tensor data: ", prompt );
        for ( u32 i = 0; i < t->ele_total; i++ ) {
                f32 f = t->data[i];
                if ( i < MAX_ELE_DISPLAY ) {
                        if ( i % 6 == 0 ) printf( "\n\t" );
                        printf( "%g, ", f );
                }
                if ( i == MAX_ELE_DISPLAY || ( t->ele_total < MAX_ELE_DISPLAY &&
                                               i == t->ele_total - 1 ) )
                        printf( "\n" );
        }
}

/* === Utils to read tensor data file --------------------------------------- */

void
read_tensor_cnt( int fd, u32 *cnt )
{
        ssize_t c = read( fd, cnt, 4 );
        if ( c < 0 ) PANIC( "failed to read bytes for a u32" );
        assert( *cnt <= MAX_TENSOR_LIMIT );
        printf( "tensor count %u\n", *cnt );
}

void
read_shape( int fd, Tensor *t )
{
        u32     dim;
        ssize_t c = read( fd, &dim, 4 );
        if ( c < 0 ) PANIC( "failed to read bytes for a u32" );
        assert( dim <= MAX_DIM_LIMIT );
        printf( "dim %u\n", dim );
        t->dim = dim;

        u32 ele_total = 1;
        u32 ele;
        printf( "shape <" );
        for ( u32 i = 0; i < dim; i++ ) {
                ssize_t c = read( fd, &ele, 4 );
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
        for ( u32 i = 0; i < t->ele_total; i++ ) {
                ssize_t c = read( fd, &f, 4 );
                if ( c < 0 ) PANIC( "failed to read bytes for a f32" );

                t->data[i] = f;

                if ( i < MAX_ELE_DISPLAY ) {
                        if ( i % 6 == 0 ) printf( "\n\t" );
                        printf( "%g, ", f );
                }
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

/* === NN ------------------------------------------------------------------- */

void
conv2d1chl( f32 *out_ptr,    /* Ptr to the output */
            f32 *data_ptr,   /* Ptr to the input */
            i32  h,          /* Input height */
            i32  w,          /* Input weight */
            f32 *kernel_ptr, /* Ptr to the kernel */
            i32  kh,         /* Kernel height */
            i32  kw          /* Kernel weight */
)
{
        i32  half_kh           = ( kh - 1 ) >> 1;
        i32  half_kw           = ( kw - 1 ) >> 1;
        i32  offset_k          = half_kh * kw + half_kw;
        f32 *kernel_center_ptr = kernel_ptr + offset_k;
        for ( i32 y = 0; y < h; y++ ) {
                f32 *input_base_ptr  = data_ptr + y * w;
                f32 *output_base_ptr = out_ptr + y * w;
                for ( i32 x = 0; x < w; x++ ) {
                        // This line is wrong
                        f32 v = *( input_base_ptr + x ) * *kernel_center_ptr;
                        for ( i32 ky = 0; ky <= half_kh; ky++ ) {
                                for ( i32 kx = 0; kx <= half_kw; kx++ ) {
                                        if ( ky == 0 && kx == 0 ) continue;

                                        v += ( ky <= y && kx <= x )
                                                 ? kernel_center_ptr[-ky * kw -
                                                                     kx] *
                                                       input_base_ptr[x -
                                                                      ky * w -
                                                                      kx]
                                                 : 0;
                                        v += ( ky + y < h && kx + x < w )
                                                 ? kernel_center_ptr[ky * kw +
                                                                     kx] *
                                                       input_base_ptr[x +
                                                                      ky * w +
                                                                      kx]
                                                 : 0;
                                }
                        }
                        /* Addeditive */
                        output_base_ptr[x] += v;
                }
        }
}

void
conv2d( Tensor *input, Tensor *weight, Tensor *bias )
{
        assert( input->dim == 4 );
        assert( weight->dim == 4 );
        assert( bias->dim == 1 );

        /* We assume batch size is 1 now. */
        assert( input->shape[0] == 1 );
        assert( input->shape[1] == weight->shape[1] );
        assert( weight->shape[0] == bias->shape[0] );
        assert( weight->shape[2] % 2 == 1 );
        assert( weight->shape[3] % 2 == 1 );

        u32 c_in     = input->shape[1];
        u32 c_out    = weight->shape[0];
        u32 h        = input->shape[2];
        u32 w        = input->shape[3];
        u32 kernel_h = weight->shape[2];
        u32 kernel_w = weight->shape[3];

        /* TODO make output an arg */
        Tensor output_tensor = {
            .dim = 4, .shape = { 1, c_out, h, w },
                 .ele_total = c_out * h * w
        };
        f32 *out_buf = malloc( sizeof( f32 ) * c_out * h * w );
        assert( out_buf != NULL );
        output_tensor.data = out_buf;

        /* Naive algorithm. */
        for ( u32 out = 0; out < c_out; out++ ) {
                f32 *kernel_ptr_base =
                    weight->data + out * c_in * kernel_h * kernel_w;
                f32 bias_v = bias->data[out];

                f32 *out_ptr = out_buf + out * h * w;

                for ( u32 i = 0; i < h * w; i++ ) {
                        out_ptr[i] = bias_v;
                }
                for ( u32 in = 0; in < c_in; in++ ) {
                        f32 *input_ptr = input->data + in * h * w;
                        f32 *kernel_ptr =
                            kernel_ptr_base + in * kernel_h * kernel_w;
                        // DEBUG printf("c_out %d, c_in %d\n", out, in);
                        conv2d1chl( out_ptr, input_ptr, (i32)h, (i32)w,
                                    kernel_ptr, (i32)kernel_h, (i32)kernel_w );
                }
        }
        show_tensor( &output_tensor, "output" );
}

/* === Main ----------------------------------------------------------------- */

int
main( void )
{
        int fd = open( BIN_DATA_FILE, O_RDONLY );
        if ( fd == -1 ) PANIC( "failed to open tensor data file" );

        u32    tensor_cnt;
        Tensor tensors[MAX_TENSOR_LIMIT];
        read_tensor_data( fd, &tensor_cnt, tensors );

        conv2d( &tensors[0], &tensors[1], &tensors[2] );
        show_tensor( &tensors[3], "target" );

        free_tensor_data( tensor_cnt, tensors );
        close( fd );
}
