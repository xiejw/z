/*
 * This code is to test the accuracy and performance of different conv2d
 * implementation.  Right now it has a naive implementation and an im2col +
 * gemm implementation.
 *
 * To run, make bench.
 */
// #include <Accelerate/Accelerate.h>
#define _POSIX_C_SOURCE 200112L
#include <assert.h>
#include <blis.h>
#include <cblas.h>
#include <fcntl.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#define ITERATIONS      100
#define MAX_ELE_DISPLAY 20    /* Max number of elements to display. */
#define MAX_DIM_LIMIT   5     /* Max dim for tenshor shape. */
#define REL_ERROR       1e-2f /* Max relative error allowed during assertion. */

#define PANIC( msg )                 \
        do {                         \
                printf( "panic\n" ); \
                printf( msg );       \
                exit( 1 );           \
        } while ( 0 )

#define RESET_TENSOR( t )         \
        do {                      \
                free_tensor( t ); \
                ( t ) = NULL;     \
        } while ( 0 )

typedef float    f32;
typedef double   f64;
typedef uint32_t u32;
typedef int32_t  i32;

typedef struct {
        u32  dim;
        u32  shape[MAX_DIM_LIMIT];
        u32  ele_total;
        f32 *data;
} Tensor;

/* === Utils to operate tensorsr -------------------------------------------- */

/* Display tensor elements in stdout after prompt. Number of elements to be
 * displayed is limited by MAX_ELE_DISPLAY.
 */
void
show_tensor( Tensor *t, const char *prompt )
{
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
        Tensor *t = malloc( sizeof( *t ) );
        assert( t != NULL );
        t->dim = dim;
        memcpy( t->shape, shape, sizeof( u32 ) * dim );
        *dst = t;

        u32 ele_total = 1;
        for ( u32 i = 0; i < dim; i++ ) {
                ele_total *= shape[i];
        }
        t->ele_total = ele_total;

        f32 *buf = malloc( sizeof( f32 ) * t->ele_total );
        assert( buf != NULL );
        t->data = buf;
}

void
dup_tensor( Tensor **dst, Tensor *src, int copy_data )
{
        Tensor *t = malloc( sizeof( *t ) );
        assert( t != NULL );
        memcpy( t, src, sizeof( *t ) );
        *dst = t;

        f32 *buf = malloc( sizeof( f32 ) * t->ele_total );
        assert( buf != NULL );
        t->data = buf;

        if ( !copy_data ) return;

        memcpy( buf, src->data, sizeof( f32 ) * t->ele_total );
}

/* Assert tensors a and b are almost same. Relative error is controled by
 * REL_ERROR.
 */
void
assert_tensors_equal( Tensor *a, Tensor *b )
{
        if ( a->ele_total != b->ele_total )
                PANIC( "assert_tensors_equal ele_total" );

        for ( u32 i = 0; i < a->ele_total; i++ ) {
                f32 err = a->data[i] - b->data[i];
                if ( fabsf( err / a->data[i] ) >= REL_ERROR ) {
                        printf( "the %u-th ele is not same. %g vs %g\n", i,
                                a->data[i], b->data[i] );
                        fflush( stdout );
                        PANIC( "assert_tensors_equal" );
                }
        }
}

/* Free a tensor on heap. */
void
free_tensor( Tensor *p )
{
        if ( p == NULL ) return;
        free( p->data );
        free( p );
}

/* === NN -------------------------------------------------------------------
 */

/* Helper util to fill the output tensor (one channel 'chl') only by doing
 * conv2d of the 1 channel input with kernel. */
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
                        f32 v = 0.f;
                        for ( i32 ky = -half_kh; ky <= half_kh; ky++ ) {
                                i32 offset_y = y + ky;
                                if ( offset_y < 0 || offset_y >= h ) continue;
                                for ( i32 kx = -half_kw; kx <= half_kw; kx++ ) {
                                        i32 offset_x = x + kx;
                                        if ( offset_x < 0 || offset_x >= w )
                                                continue;
                                        v += kernel_center_ptr[ky * kw + kx] *
                                             input_base_ptr[x + kx + ky * w];
                                }
                        }
                        /* Addeditive */
                        output_base_ptr[x] += v;
                }
        }
}

/* Conv2D on a 4D (N, C_in, H, W) input tensor with
 * - (C_out, C_in, KH, KW) * kernel (weight) and
 * - (C_out) bias.
 *
 * NOTE:
 * - This naive implementation assumes batch size is 1, i.e., N=1;
 * - This naive implementation assumes conv2d is same padding, KH and KW are
 *   both odd numbers.
 */
void
conv2d( Tensor **dst, Tensor *input, Tensor *weight, Tensor *bias )
{
        assert( input->dim == 4 );
        assert( weight->dim == 4 );
        assert( bias->dim == 1 );

        assert( input->shape[1] == weight->shape[1] );
        assert( weight->shape[0] == bias->shape[0] );
        /* We assume batch size is 1 now. */
        assert( input->shape[0] == 1 );
        /* We assume kernel size (h and w) are both odd */
        assert( weight->shape[2] % 2 == 1 );
        assert( weight->shape[3] % 2 == 1 );

        u32 c_in     = input->shape[1];
        u32 c_out    = weight->shape[0];
        u32 h        = input->shape[2];
        u32 w        = input->shape[3];
        u32 kernel_h = weight->shape[2];
        u32 kernel_w = weight->shape[3];

        alloc_tensor( dst, 4, (u32[]){ 1, c_out, h, w } );
        f32 *out_buf = ( *dst )->data;

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
                        conv2d1chl( out_ptr, input_ptr, (i32)h, (i32)w,
                                    kernel_ptr, (i32)kernel_h, (i32)kernel_w );
                }
        }
}
void
fill_channel_input( f32 *out_ptr, int h, int w, int y, int x, f32 *in_ptr,
                    int kh, int kw )
{
        int half_kh = ( kh - 1 ) >> 1;
        int half_kw = ( kw - 1 ) >> 1;

        for ( int ky = -half_kh; ky <= half_kh; ky++ ) {
                int offset_y = y + ky;
                if ( offset_y < 0 || offset_y >= h ) {
                        for ( int n = 0; n < kw; n++ ) {
                                *out_ptr = 0.f;
                                out_ptr++;
                        }
                        continue;
                };
                for ( int kx = -half_kw; kx <= half_kw; kx++ ) {
                        int offset_x = x + kx;
                        if ( offset_x < 0 || offset_x >= w ) {
                                *out_ptr = 0.f;
                                out_ptr++;
                                continue;
                        };
                        *out_ptr = in_ptr[kx + ky * w];
                        out_ptr++;
                }
        }
}

void
conv2d_fast( Tensor **dst, Tensor *input, Tensor *weight, Tensor *bias,
             int use_blis )
{
        assert( input->dim == 4 );
        assert( weight->dim == 4 );
        assert( bias->dim == 1 );

        assert( input->shape[1] == weight->shape[1] );
        assert( weight->shape[0] == bias->shape[0] );
        /* We assume batch size is 1 now. */
        assert( input->shape[0] == 1 );
        /* We assume kernel size (h and w) are both odd */
        assert( weight->shape[2] % 2 == 1 );
        assert( weight->shape[3] % 2 == 1 );

        u32 c_in     = input->shape[1];
        u32 c_out    = weight->shape[0];
        u32 h        = input->shape[2];
        u32 w        = input->shape[3];
        u32 kernel_h = weight->shape[2];
        u32 kernel_w = weight->shape[3];

        alloc_tensor( dst, 4, (u32[]){ 1, c_out, h, w } );
        f32 *out_buf = ( *dst )->data;

        Tensor *rhs;
        alloc_tensor( &rhs, 2, (u32[]){ h * w, kernel_h * kernel_w * c_in } );

        // favor reading over writing
        u32 rhs_w = kernel_h * kernel_w * c_in;
        for ( u32 c = 0; c < c_in; c++ ) {
                f32 *in_ptr_base = input->data + (u32)c * h * w;

                for ( u32 row = 0; row < h; row++ ) {
                        f32 *out_ptr_base = rhs->data + ( row * w ) * rhs_w +
                                            (u32)c * kernel_h * kernel_w;
                        for ( u32 col = 0; col < w; col++ ) {
                                f32 *out_ptr = out_ptr_base + col * rhs_w;
                                f32 *in_ptr  = in_ptr_base + row * w + col;
                                fill_channel_input(
                                    out_ptr, (int)h, (int)w, (int)row, (int)col,
                                    in_ptr, (int)kernel_h, (int)kernel_w );
                        }
                }
        }

        // DEBUG check im2col
        // printf("im2col\n");
        // for ( u32 i = 0; i < rhs->ele_total; i++ ) {
        //         printf( "%.2f,  ", rhs->data[i] );
        //         if ( ( i + 1 ) % ( kernel_h * kernel_w ) == 0 ) printf( "\n"
        //         );
        // }

        // // DEBUG check kernel
        // printf("kernel\n");
        // for ( u32 i = 0; i < weight->ele_total; i++ ) {
        //         printf( "%6.2f,  ", weight->data[i] );
        //         if ( ( i + 1 ) % ( kernel_h * kernel_w ) == 0 ) printf( "\n"
        //         );
        // }

        // Fill bias
        for ( u32 c = 0; c < c_out; c++ ) {
                f32  b       = bias->data[c];
                f32 *out_ptr = out_buf + c * h * w;
                for ( u32 n = 0; n < h * w; n++ ) {
                        *out_ptr = b;
                        out_ptr++;
                }
        }

        // DEBUG check dst bias
        // printf("dst_bias\n");
        // for ( u32 i = 0; i < (*dst)->ele_total; i++ ) {
        //         printf( "%.2f,  ", (*dst)->data[i] );
        //         if ( ( i + 1 ) % ( h*w ) == 0 ) printf( "\n" );
        // }

        int K = (int)( kernel_h * kernel_w * c_in );

        if ( use_blis ) {
                float alpha = 1.0f, beta = 1.0f;
                bli_sgemm( BLIS_NO_TRANSPOSE, BLIS_TRANSPOSE, (int)c_out,
                           (int)( h * w ), K, &alpha,
                           /*A=*/weight->data, K, 1,
                           /*B=*/rhs->data, K, 1, &beta, /*C=*/out_buf,
                           (int)( h * w ), 1 );
        } else {
                /* blas */
                cblas_sgemm( CblasRowMajor, CblasNoTrans, CblasTrans,
                             (int)c_out, (int)( h * w ), K, 1.0f,
                             /*A=*/weight->data, K,
                             /*B=*/rhs->data, K, 1.0f, /*C=*/out_buf,
                             (int)( h * w ) );
        }

        RESET_TENSOR( rhs );
}

/* === Main ----------------------------------------------------------------- */

// #define C_IN  2
// #define C_OUT 2
// #define K     3
// #define H     2
// #define W     2
#define C_IN  128
#define C_OUT 128
#define K     5
#define H     6
#define W     7

f64
time_now( void )
{
        struct timeval tv;
        gettimeofday( &tv, NULL );
        return (f64)tv.tv_sec + (f64)tv.tv_usec / 1000. / 1000.;
}

void
fill_rng( Tensor *in )
{
        for ( u32 i = 0; i < in->ele_total; i++ ) {
                in->data[i] = (f32)( rand( ) % 1024 ) / 1024.f - 0.5f;
        }
}

void
compare( void )
{
        Tensor *in;
        Tensor *kernel;
        Tensor *bias;
        Tensor *output;
        Tensor *output1;
        Tensor *output2;

        alloc_tensor( &in, 4, (u32[]){ 1, C_IN, H, W } );
        alloc_tensor( &kernel, 4, (u32[]){ C_OUT, C_IN, K, K } );
        alloc_tensor( &bias, 1, (u32[]){ C_OUT } );

        fill_rng( in );
        fill_rng( kernel );
        fill_rng( bias );

        // DEBUG Manually create a input with kernel and bias to check result.
        //
        // memcpy( in->data, (f32[]){ 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0 },
        //         sizeof( f32 ) * 8 );
        // memcpy( bias->data, (f32[]){ 0.1f, 0.2f }, sizeof( f32 ) * 2 );
        // memcpy( kernel->data, (f32[]){
        //                 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0,
        //                 11.0, 12.0, 13.0, 14.0, 15.0, 16.0, 17.0, 18.0, 19.0,
        //                 -1.0,
        //                 -  2.0,-  3.0,-  4.0,-  5.0,-  6.0,-  7.0,-  8.0,
        //                 -9.0,
        //                 -11.0,- 12.0,- 13.0,- 14.0,- 15.0,- 16.0,- 17.0,- 18.0,
        //                 -19.0,
        //                 },
        //         sizeof( f32 ) * 4*9 );

        /* Naive code */
        f64 start = time_now( );

        for ( int i = 0; i < ITERATIONS; i++ ) {
                conv2d( &output, in, kernel, bias );
                // RESET_TENSOR( output );
        }
        f64 end = time_now( );
        printf( "taking %f seconds for naive code\n", ( end - start ) );

        /* BLAS code */
        start = time_now( );
        for ( int i = 0; i < ITERATIONS; i++ ) {
                conv2d_fast( &output1, in, kernel, bias, /*use_blis=*/0 );
                // RESET_TENSOR( output1 );
        }
        end = time_now( );
        printf( "taking %f seconds for blas code\n", ( end - start ) );

        /* BLIS code */
        start = time_now( );
        for ( int i = 0; i < ITERATIONS; i++ ) {
                conv2d_fast( &output2, in, kernel, bias, /*use_blis=*/1 );
                // RESET_TENSOR( output1 );
        }
        end = time_now( );
        printf( "taking %f seconds for blis code\n", ( end - start ) );

        show_tensor( output, "output" );
        show_tensor( output1, "blas_output" );
        show_tensor( output2, "blis_output" );
        assert_tensors_equal( output, output1 );
        assert_tensors_equal( output, output2 );

        RESET_TENSOR( in );
        RESET_TENSOR( output );
        RESET_TENSOR( output1 );
        RESET_TENSOR( output2 );
        RESET_TENSOR( kernel );
        RESET_TENSOR( bias );
}

int
main( void )
{
        srand( (unsigned)time( NULL ) );
        compare( );
}
