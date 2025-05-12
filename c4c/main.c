#include <assert.h>
#include <fcntl.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* === BLAS related header and kernels -------------------------------------- */

#ifdef MACOS_ACCELERATE
#include <Accelerate/Accelerate.h>
#endif

#ifdef BLAS
#include <cblas.h>
#endif

#if defined( MACOS_ACCELERATE ) || defined( BLAS )
#define conv2d conv2d_blas
#else
#define conv2d conv2d_naive
#endif

/* === Configurations and Macros -------------------------------------------- */

typedef float    f32;
typedef uint32_t u32;
typedef int32_t  i32;

#define ROWS 6
#define COLS 7

#define BIN_DATA_FILE    ".build/tensor_data.bin" /* Tensor data dump file */
#define MAX_DIM_LIMIT    5      /* Max dim for tensor shape. */
#define MAX_TENSOR_LIMIT 128    /* Max number of tensors. */
#define MAX_ELE_DISPLAY  20     /* Max number of elements to display. */
#define BN_EPS           0.001f /* EPS for Batch norm. */

#ifndef MCTS_ITER_CNT
#define MCTS_ITER_CNT 1600 /* Simulation iteration count. */
#endif

#define DISABLE_SHOW_TENSOR 1

#define DEBUG \
        if ( !DISABLE_SHOW_TENSOR ) printf

#define PANIC( msg )                 \
        do {                         \
                printf( "panic\n" ); \
                printf( msg );       \
                exit( -1 );          \
        } while ( 0 )

#define COL_ROW_TO_IDX( col, row ) ( ( row ) * COLS + ( col ) )

#define RESET_TENSOR( t )         \
        do {                      \
                free_tensor( t ); \
                ( t ) = NULL;     \
        } while ( 0 )

/* === Tensor and NN related data structures -------------------------------- */

typedef struct {
        u32  dim;
        u32  shape[MAX_DIM_LIMIT];
        u32  ele_total;
        f32 *data;
} Tensor;

typedef struct {
        u32    weight_cnt;                /* Total number of weights. */
        Tensor weights[MAX_TENSOR_LIMIT]; /* All weights. */
} NN;

/* === Game play related data structures ------------------------------------ */

typedef enum { NA, BLACK, WHITE } Color;

typedef struct {
        Color board[ROWS * COLS];
        Color next_player;
        Color nn_player;
} Game;

/* === Utils to operate tensor ---------------------------------------------- */

/* Display tensor elements in stdout after prompt. Number of elements to be
 * displayed is limited by MAX_ELE_DISPLAY.
 */
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

/* Allocates a tensor with shape and dim but random data buffer. */
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

/* Dup a tensor with the same shape and dim as src. If copy_data is non-zero,
 * the data buffer is copied as well.
 */
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

/* Free a tensor on heap. */
void
free_tensor( Tensor *p )
{
        if ( p == NULL ) return;
        free( p->data );
        free( p );
}

/* Free tensor data inside a static allocated tensor array (tensors). */
void
free_static_tensor_data( u32 tensor_cnt, Tensor *tensors )
{
        for ( u32 i = 0; i < tensor_cnt; i++ ) {
                free( tensors[i].data );
        }
}

/* === Utils to read tensor data file --------------------------------------- */

/* Tensor data file specification.
 *
 * The file contains tensors only, no operation graphs/dags.
 * - The first 4 bytes is little endian unsigned int32 (u32), which indicates
 *   the total number of tensors in this file.
 * - After the first 4 bytes, all tensors' shapes are recorded continuously (no
 *   data is recorded in this section). Each shape starts with dimension (dim)
 *   in u32, followed by shape element each is u32. For example, if two tensors
 *   are stored in file with shape {2, 3} and {1, 3, 5}, the shapes are
 *   recorded as a sequence of u32s: 2233135.
 * - Immediately after last section, all tensor data are stored as float32 (f32)
 *   bytes, without any delimiters. This makes mmap easier during file loading.
 *
 * Read read_tensor_data function for details.
 */

void
read_tensor_cnt( int fd, u32 *cnt )
{
        ssize_t c = read( fd, cnt, 4 );
        if ( c < 0 ) PANIC( "failed to read bytes for a u32" );
        assert( *cnt <= MAX_TENSOR_LIMIT );
        DEBUG( "tensor count %u\n", *cnt );
}

void
read_shape( int fd, Tensor *t )
{
        u32     dim;
        ssize_t c = read( fd, &dim, 4 );
        if ( c < 0 ) PANIC( "failed to read bytes for a u32" );
        assert( dim <= MAX_DIM_LIMIT );
        DEBUG( "dim %u\n", dim );
        t->dim = dim;

        u32 ele_total = 1;
        u32 ele;
        DEBUG( "shape <" );
        for ( u32 i = 0; i < dim; i++ ) {
                ssize_t c = read( fd, &ele, 4 );
                if ( c < 0 ) PANIC( "failed to read bytes for a u32" );

                DEBUG( "%u, ", ele );
                t->shape[i] = ele;
                ele_total *= ele;
        }
        t->ele_total = ele_total;
        DEBUG( ">\n" );
        DEBUG( "ele_total: %u\n", ele_total );
}

void
read_tensor( int fd, Tensor *t )
{
        size_t byte_count = sizeof( f32 ) * t->ele_total;
        t->data           = malloc( byte_count );
        assert( t->data != NULL );

        ssize_t c = read( fd, t->data, 4 * t->ele_total );
        if ( c < 0 ) PANIC( "failed to read bytes for a f32" );
        if ( c != 4 * t->ele_total ) PANIC( "failed to read full tensor" );

        show_tensor( t, "tensor data" );
}

void
read_tensor_data( const char *file_name, u32 *tensor_cnt, Tensor *weights )
{
        int fd = open( file_name, O_RDONLY );
        if ( fd == -1 ) PANIC( "failed to open tensor data file" );

        read_tensor_cnt( fd, tensor_cnt );

        /* Pass 1: Read shapes */
        for ( u32 i = 0; i < *tensor_cnt; i++ ) {
                read_shape( fd, &weights[i] );
        }
        /* Pass 2: Read tensor data */
        for ( u32 i = 0; i < *tensor_cnt; i++ ) {
                read_tensor( fd, &weights[i] );
        }
        close( fd );
}

/* === NN -------------------------------------------------------------------
 *
 * See reference_model.py for the PyTorch reference model.
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
                        /* Additive */
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
conv2d_naive( Tensor **dst, Tensor *input, Tensor *weight, Tensor *bias )
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

#if defined( MACOS_ACCELERATE ) || defined( BLAS )

/* A helper method of conv2d_blas to extract a block of (kernel_h x kernel_w)
 * from input and fill a line into the im2col matrix. If the input is out of
 * box, fill 0.f instead.
 *
 * NOTE:
 * - This implementation assumes kh and kw are odd.
 * - out_ptr points to the start of the line to be filled.
 * - in_ptr points the **center** of the input image.
 */
void
conv2d_blas_fill_channel_input( f32 *out_ptr, int h, int w, int y, int x,
                                f32 *in_ptr, int kh, int kw )
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

/* Conv2D on a 4D (N, C_in, H, W) input tensor with
 * - (C_out, C_in, KH, KW) * kernel (weight) and
 * - (C_out) bias.
 *
 * NOTE:
 * - This implementation assumes batch size is 1, i.e., N=1;
 * - This implementation assumes conv2d is same padding, KH and KW are
 *   both odd numbers.
 * - This implementation uses im2col and cblas_sgemm to do the trick for
 *   speeding up.
 */
void
conv2d_blas( Tensor **dst, Tensor *input, Tensor *weight, Tensor *bias )
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

        /* === im2col ----------------------------------------------------------
         *
         * The kernel K (weight) shape is (c_out, c_in*kh*kw).
         * We construct a new matrix B with shape (h*w, c_in*kh*kw) and do
         * matmul(K, trans(B)) + bias, the result is the output.
         *
         * The idea is to extract the kernel block size patch from input and
         * fill it in the matrix B one feature channel after another. Then the
         * contracting dimension of the matmul is in fact the conv2d cross all
         * input channels. Smart.
         */

        Tensor *col_matrix;
        alloc_tensor( &col_matrix, 2,
                      (u32[]){ h * w, kernel_h * kernel_w * c_in } );

        // im2col: Favor reading over writing. For each feature channel, fill
        // the output pixel by extracting input patch.
        const u32 matrix_w = kernel_h * kernel_w * c_in;
        for ( u32 c = 0; c < c_in; c++ ) {
                f32 *in_ptr_base = input->data + (u32)c * h * w;

                for ( u32 row = 0; row < h; row++ ) {
                        f32 *out_ptr_base = col_matrix->data +
                                            ( row * w ) * matrix_w +
                                            (u32)c * kernel_h * kernel_w;
                        for ( u32 col = 0; col < w; col++ ) {
                                f32 *out_ptr = out_ptr_base + col * matrix_w;
                                f32 *in_ptr  = in_ptr_base + row * w + col;
                                conv2d_blas_fill_channel_input(
                                    out_ptr, (int)h, (int)w, (int)row, (int)col,
                                    in_ptr, (int)kernel_h, (int)kernel_w );
                        }
                }
        }

        /* Fill bias into the output buffer. */
        for ( u32 c = 0; c < c_out; c++ ) {
                f32  b       = bias->data[c];
                f32 *out_ptr = out_buf + c * h * w;
                for ( u32 n = 0; n < h * w; n++ ) {
                        *out_ptr = b;
                        out_ptr++;
                }
        }

        /* === The magic: matmul ---------------------------------------------*/
        int K = (int)( kernel_h * kernel_w * c_in );
        cblas_sgemm( CblasRowMajor, CblasNoTrans, CblasTrans, (int)c_out,
                     (int)( h * w ), K, 1.0f,
                     /*A=*/weight->data, K,
                     /*B=*/col_matrix->data, K, 1.0f, /*C=*/out_buf,
                     (int)( h * w ) );

        RESET_TENSOR( col_matrix );
}

#endif  // defined(MACOS_ACCELERATE) || defined(BLAS)

/* Batch norm on a 4D (N, C, H, W) input tensor.
 *
 * In case of CNNs the mean/variance should be taken across all pixels over the
 * batch for each input channel. In other words, if your input is of shape (N,
 * C, H, W) , your mean/variance will be of shape  (1, C, 1, 1) or (C) . The
 * reason for that is that the weights of a kernel in a CNN are shared in a
 * spatial dimension (HxW). However, in the channel dimension C the weights are
 * not shared.
 *
 * NOTE: This naive implementation assumes batch size is 1, i.e., N=1;
 */
void
batchnorm2d( Tensor **dst, Tensor *input, Tensor *weight, Tensor *bias,
             Tensor *mean, Tensor *var )
{
        assert( input->dim == 4 );
        assert( weight->dim == 1 );
        assert( bias->dim == 1 );
        assert( mean->dim == 1 );
        assert( var->dim == 1 );

        /* We assume batch size is 1 now. */
        assert( input->shape[0] == 1 );
        u32 num_features = input->shape[1];
        assert( num_features == weight->shape[0] );
        assert( num_features == bias->shape[0] );
        assert( num_features == mean->shape[0] );
        assert( num_features == var->shape[0] );

        dup_tensor( dst, input, 0 );
        f32 *out_buf = ( *dst )->data;

        u32 h        = input->shape[2];
        u32 w        = input->shape[3];
        u32 img_size = h * w;

        for ( u32 n = 0; n < num_features; n++ ) {
                f32 *input_base_ptr  = input->data + n * img_size;
                f32 *output_base_ptr = out_buf + n * img_size;

                f32 w            = weight->data[n];
                f32 b            = bias->data[n];
                f32 m            = mean->data[n];
                f32 v            = var->data[n];
                f32 inv_sqrt_v_w = 1.f / sqrtf( v + BN_EPS ) * w;
                f32 true_bias    = -m * inv_sqrt_v_w + b;

                /* o = (i - m ) / sqrt(v + eps) * w + b
                 *   = (i - m ) * (1 / sqrt(v+eps) * w) + b
                 *   = (i - m ) * inv_sqrt_v_w + b
                 *   = i * inv_sqrt_v_w + (- m * inv_sqrt_v_w + b)
                 *   = i * inv_sqrt_v_w + true_bias
                 */

                for ( u32 i = 0; i < img_size; i++ ) {
                        output_base_ptr[i] =
                            input_base_ptr[i] * inv_sqrt_v_w + true_bias;
                }
        }
}

/* Relu performs max(0, x) for each element x. For performance, this layer does
 * in place update.
 *
 * NOTE: We could fuse this with previous layer to avoid one more mem reading.
 */
void
relu_inplace( Tensor *t )
{
        u32 size = t->ele_total;
        for ( u32 i = 0; i < size; i++ ) {
                f32 f = t->data[i];
                if ( f < 0 ) t->data[i] = 0.f;
        }
}

/* Add is the Residual add in the resnet block. For performance, this layer does
 * in place update to the dst.
 *
 * NOTE: We could fuse this with previous layer to avoid one more mem reading.
 */
void
add_inplace( Tensor *dst, Tensor *src )
{
        assert( dst->ele_total == src->ele_total );
        u32 size = dst->ele_total;
        for ( u32 i = 0; i < size; i++ ) {
                dst->data[i] += src->data[i];
        }
}

/* Perform softmax on the 1-st dim (0-based). For performance, this layer does
 * in place update.
 *
 * NOTE:
 * - batch size is assume to be 1.
 */
void
softmax_inplace( Tensor *dst )
{
        assert( dst->dim == 2 );
        assert( dst->shape[0] == 1 );
        u32  ele_cnt = dst->ele_total;
        f32 *ptr     = dst->data;
        f32  max     = ptr[0];
        for ( u32 i = 1; i < ele_cnt; i++ ) {
                f32 v = ptr[i];
                if ( v > max ) max = v;
        }
        f32 total = 0.f;
        for ( u32 i = 0; i < ele_cnt; i++ ) {
                f32 v = expf( ptr[i] - max );
                total += v;
                ptr[i] = v;
        }
        for ( u32 i = 0; i < ele_cnt; i++ ) {
                ptr[i] /= total;
        }
}

/* Tanh performs element wise tanh op on input. */
void
tanh_inplace( Tensor *dst )
{
        u32  ele_cnt = dst->ele_total;
        f32 *ptr     = dst->data;
        for ( u32 i = 0; i < ele_cnt; i++ ) {
                f32 v = ptr[i];
                ptr[i] =
                    ( expf( v ) - expf( -v ) ) / ( expf( v ) + expf( -v ) );
        }
}

/* Linear layer to perform matmul on (1, C) x (C, N) = (1, N).
 *
 * NOTE
 * - Batch size is assumed to be 1.
 * - The weight matrix is assumed to have shape (N, C) rather than (C, N)
 * - Input is OK to have more than 2 dim, and we implicitly do a flatten.
 */
void
linear( Tensor **dst, Tensor *input, Tensor *weight, Tensor *bias )
{
        assert( input->dim >= 2 );
        assert( weight->dim == 2 );
        assert( bias->dim == 1 );

        /* An implicit flatten. */
        u32 ele_cnt = 1;
        for ( u32 i = 1; i < input->dim; i++ ) {
                ele_cnt *= input->shape[i];
        }

        assert( 1 == input->shape[0] );
        assert( ele_cnt == weight->shape[1] );
        assert( weight->shape[0] == bias->shape[0] );

        u32 out_dim = weight->shape[0];

        alloc_tensor( dst, 2, (u32[]){ 1, out_dim } );
        f32 *out_buf = ( *dst )->data;

        f32 *input_ptr = input->data;
        for ( u32 n = 0; n < out_dim; n++ ) {
                f32  v          = 0.f;
                f32 *weight_ptr = weight->data + n * ele_cnt;
                for ( u32 i = 0; i < ele_cnt; i++ ) {
                        v += input_ptr[i] * weight_ptr[i];
                }
                out_buf[n] = v + bias->data[n];
        }
}

void
resnet_block( Tensor **dst, Tensor *src, u32 *weight_idx, Tensor *weights )
{
        Tensor *dup    = NULL;
        Tensor *input  = src;
        Tensor *output = NULL;

        dup_tensor( &dup, src, /*copy_data=*/1 );

        conv2d( &output, input, &weights[*weight_idx + 0],
                &weights[*weight_idx + 1] );
        *weight_idx += 2;

        input = output;
        batchnorm2d( &output, input, &weights[*weight_idx + 0],
                     &weights[*weight_idx + 1], &weights[*weight_idx + 2],
                     &weights[*weight_idx + 3] );
        *weight_idx += 4;
        RESET_TENSOR( input );

        relu_inplace( output );

        input = output;
        conv2d( &output, input, &weights[*weight_idx + 0],
                &weights[*weight_idx + 1] );
        *weight_idx += 2;
        RESET_TENSOR( input );

        input = output;
        batchnorm2d( &output, input, &weights[*weight_idx + 0],
                     &weights[*weight_idx + 1], &weights[*weight_idx + 2],
                     &weights[*weight_idx + 3] );
        *weight_idx += 4;
        RESET_TENSOR( input );

        add_inplace( output, dup );
        RESET_TENSOR( dup );

        relu_inplace( output );
        *dst = output;
}

void
policy_head( Tensor **dst, Tensor *src, u32 *weight_idx, Tensor *weights )
{
        Tensor *input  = src;
        Tensor *output = NULL;

        conv2d( &output, input, &weights[*weight_idx + 0],
                &weights[*weight_idx + 1] );
        *weight_idx += 2;

        input = output;
        batchnorm2d( &output, input, &weights[*weight_idx + 0],
                     &weights[*weight_idx + 1], &weights[*weight_idx + 2],
                     &weights[*weight_idx + 3] );
        *weight_idx += 4;
        RESET_TENSOR( input );

        relu_inplace( output );

        input = output;
        linear( &output, input, &weights[*weight_idx + 0],
                &weights[*weight_idx + 1] );
        *weight_idx += 2;
        RESET_TENSOR( input );

        softmax_inplace( output );

        *dst = output;
}

void
value_head( Tensor **dst, Tensor *src, u32 *weight_idx, Tensor *weights )
{
        Tensor *input  = src;
        Tensor *output = NULL;

        conv2d( &output, input, &weights[*weight_idx + 0],
                &weights[*weight_idx + 1] );
        *weight_idx += 2;

        input = output;
        batchnorm2d( &output, input, &weights[*weight_idx + 0],
                     &weights[*weight_idx + 1], &weights[*weight_idx + 2],
                     &weights[*weight_idx + 3] );
        *weight_idx += 4;
        RESET_TENSOR( input );

        relu_inplace( output );

        input = output;
        linear( &output, input, &weights[*weight_idx + 0],
                &weights[*weight_idx + 1] );
        *weight_idx += 2;
        RESET_TENSOR( input );

        relu_inplace( output );

        input = output;
        linear( &output, input, &weights[*weight_idx + 0],
                &weights[*weight_idx + 1] );
        *weight_idx += 2;
        RESET_TENSOR( input );

        tanh_inplace( output );

        *dst = output;
}

/* Convert game board to feature input.
 *
 * The input tensor specification:
 *
 * - For 3 channel input with shape {1,3,ROWS,COLS}, all f32 data are
 *   zero by default.
 * - If the next_player is BLACK, the 3rd channel, i.e., [1,2,:,:] are
 *   set to 1.
 * - For each stone on the board, if it is BLACK, the corresponding f32
 *   data in the 1st channel is 1, i.e., [1,0,row,col] = 1. If WHITE,
 *   the 2nd channel for that data is 1, i.e, [1,1,row,col] = 1.
 */
void
convert_game_to_tensor_input( Tensor **dst, Game *g )
{
        Tensor *in;
        alloc_tensor( &in, 4, (u32[]){ 1, 3, ROWS, COLS } );
        memset( in->data, 0, sizeof( f32 ) * 3 * ROWS * COLS );

        if ( g->next_player == BLACK ) {
                f32 *ptr = in->data + 2 * ROWS * COLS;
                for ( int i = 0; i < ROWS * COLS; i++ ) ptr[i] = 1.0f;
        }

        Color *board     = g->board;
        f32   *black_ptr = in->data;
        f32   *white_ptr = in->data + 1 * ROWS * COLS;
        for ( int row = 0; row < ROWS; row++ ) {
                for ( int col = 0; col < COLS; col++ ) {
                        int   idx = COL_ROW_TO_IDX( col, row );
                        Color c   = board[idx];
                        if ( c == NA ) continue;
                        if ( c == BLACK ) {
                                black_ptr[idx] = 1.0f;
                        } else {
                                white_ptr[idx] = 1.0f;
                        }
                }
        }
        *dst = in;
}

NN *
nn_new( const char *data_file )
{
        NN *nn = malloc( sizeof( *nn ) );
        assert( nn != NULL );
        nn->weight_cnt = 0;
        read_tensor_data( data_file, &nn->weight_cnt, nn->weights );
        return nn;
}

void
nn_free( NN *p )
{
        if ( p == NULL ) return;
        free_static_tensor_data( p->weight_cnt, p->weights );
        free( p );
}

void
nn_forward( NN *nn, Tensor *in, Tensor **policy_out, Tensor **value_out )
{
        Tensor *input  = NULL;
        Tensor *output = NULL;

        u32 weight_idx = 0;

        /* Layer 0 */
        input = in;
        conv2d( &output, in, &nn->weights[weight_idx + 0],
                &nn->weights[weight_idx + 1] );
        weight_idx += 2;

        /* Layer 1 */
        input = output;
        batchnorm2d( &output, input, &nn->weights[weight_idx + 0],
                     &nn->weights[weight_idx + 1], &nn->weights[weight_idx + 2],
                     &nn->weights[weight_idx + 3] );
        weight_idx += 4;

        /* Layer 2 */
        RESET_TENSOR( input );
        relu_inplace( output );

        /* Block 0 */
        RESET_TENSOR( input );
        input = output;
        resnet_block( &output, input, &weight_idx, nn->weights );

        /* Block 1 */
        RESET_TENSOR( input );
        input = output;
        resnet_block( &output, input, &weight_idx, nn->weights );

        /* Block 2 */
        RESET_TENSOR( input );
        input = output;
        resnet_block( &output, input, &weight_idx, nn->weights );

        /* Block 3 */
        RESET_TENSOR( input );
        input = output;
        resnet_block( &output, input, &weight_idx, nn->weights );

        /* Block 4 */
        RESET_TENSOR( input );
        input = output;
        resnet_block( &output, input, &weight_idx, nn->weights );

        /* Policy Head */
        RESET_TENSOR( input );
        input = output; /* This tensor will be re-used by value head. */
        policy_head( &output, input, &weight_idx, nn->weights );
        *policy_out = output;
        output      = NULL;

        /* Value Head */
        value_head( &output, input, &weight_idx, nn->weights );
        *value_out = output;
        output     = NULL;
        RESET_TENSOR( input );

        /* Output tensor must be the final one. */
        assert( weight_idx == nn->weight_cnt );
}

/* === MCTS node and tree --------------------------------------------------- */

Game *game_dup_snapshot( Game *g );
int   game_legal_row( Game *g, int col );
int   game_winner( Game *g );
void  game_free( Game *g );

typedef struct MCTSNode {
        Game *game_snapshot; /* Owned */
        NN   *nn;            /* Unowned */
        int   total_count;
        f32   predicated_reward;
        /* Owned children for each legal move. NULL means unexpanded yet. */
        struct MCTSNode *c[COLS];
        /* Visited count for each legal move. -1 for illegal column. */
        int n[COLS];
        /* Backed up total value for each legal move. */
        f32 w[COLS];
        /* Prior probability for each legal move. */
        f32 p[COLS];
} MCTSNode;

MCTSNode *
mcts_node_new( /*moved_in*/ Game *game_snapshot, NN *nn )
{
        MCTSNode *node = calloc( 1, sizeof( *node ) );
        assert( node != NULL );
        node->game_snapshot = game_snapshot; /* owned now */
        node->nn            = nn;

        Tensor *in;
        Tensor *policy_out;
        Tensor *value_out;
        convert_game_to_tensor_input( &in, game_snapshot );
        nn_forward( nn, in, &policy_out, &value_out );

        node->predicated_reward = value_out->data[0];

        for ( int col = 0; col < COLS; col++ ) {
                int row = game_legal_row( game_snapshot, col );
                if ( row == -1 ) { /* illegal column. */
                        node->n[col] = -1;
                        continue;
                }

                node->p[col] = policy_out->data[COL_ROW_TO_IDX( col, row )];
        }
        RESET_TENSOR( in );
        RESET_TENSOR( policy_out );
        RESET_TENSOR( value_out );
        return node;
}

void
mcts_node_free( MCTSNode *n )
{
        if ( n == NULL ) return;
        game_free( n->game_snapshot );
        for ( int i = 0; i < COLS; i++ ) {
                mcts_node_free( n->c[i] );
        }
        free( n );
}

int
mcts_node_select_next_col_to_evaluate( MCTSNode *node )
{
        const f32 c                = 1.0f;
        const f32 sqrt_total_count = sqrtf( (f32)node->total_count );
        int       col_to_evaluate  = -1;
        f32       best_q           = 0;
        for ( int col = 0; col < COLS; col++ ) {
                int n = node->n[col];
                if ( n == -1 ) continue; /* illegal col. */
                f32 q = node->w[col] / ( n > 0 ? (f32)n : 1.f );
                q += c * node->p[col] * sqrt_total_count / ( 1.0f + (f32)n );

                if ( col_to_evaluate == -1 || q > best_q ) {
                        col_to_evaluate = col;
                        best_q          = q;
                }
        }
        assert( col_to_evaluate != -1 );
        return col_to_evaluate;
}

int
mcts_node_select_next_col_to_play( MCTSNode *node )
{
        int col_to_evaluate = -1;
        int best_n          = 0;
        for ( int col = 0; col < COLS; col++ ) {
                int n = node->n[col];
                if ( n == -1 ) continue; /* illegal col. */

                // DEBUG info
                printf( "col %d n %4d p %7.3f avg(w) %7.3f \n", col, n,
                        node->p[col], node->w[col] / (f32)( n > 0 ? n : 1 ) );
                if ( col_to_evaluate == -1 || n > best_n ) {
                        col_to_evaluate = col;
                        best_n          = n;
                }
        }
        assert( col_to_evaluate != -1 );
        return col_to_evaluate;
}

void
mcts_node_backup_reward( MCTSNode *n, int col, f32 reward )
{
        assert( n->n[col] != -1 );
        n->total_count++;
        n->n[col] += 1;
        n->w[col] += reward;
}

#define MAX_MCTS_SIMULATE_PATH_LEN ( ROWS * COLS )

void
mcts_backup_rewards( f32 black_reward, f32 white_reward, int count,
                     MCTSNode **simulate_path_node, int *simulate_path_col )
{
        for ( int i = 0; i < count; i++ ) {
                MCTSNode *n = simulate_path_node[i];
                if ( n->game_snapshot->next_player == BLACK ) {
                        mcts_node_backup_reward( n, simulate_path_col[i],
                                                 black_reward );
                } else {
                        mcts_node_backup_reward( n, simulate_path_col[i],
                                                 white_reward );
                }
        }
}

void
mcts_run_simulation( MCTSNode *root, int iterations )
{
        /* Record path of the simulation for backing up rewards. */
        int       simulate_len;
        MCTSNode *simulate_path_node[MAX_MCTS_SIMULATE_PATH_LEN];
        int       simulate_path_col[MAX_MCTS_SIMULATE_PATH_LEN];

        /* Progress report */
        time_t last_report_progress = time( NULL );

        for ( int it = 0; it < iterations; it++ ) {
                MCTSNode *node = root;

                simulate_len = 0;

                while ( 1 ) {
                        int col = mcts_node_select_next_col_to_evaluate( node );
                        int row = game_legal_row( node->game_snapshot, col );
                        assert( row != -1 );
                        simulate_path_node[simulate_len] = node;
                        simulate_path_col[simulate_len]  = col;
                        simulate_len++;

                        Color next_player =
                            node->game_snapshot->next_player == BLACK ? WHITE
                                                                      : BLACK;
                        Game *dup_game =
                            game_dup_snapshot( node->game_snapshot );
                        dup_game->board[COL_ROW_TO_IDX( col, row )] =
                            dup_game->next_player;
                        dup_game->next_player = next_player;

                        int winner = game_winner( dup_game );

                        /* Found winner */
                        if ( winner >= 0 ) {
                                f32 black_reward;
                                f32 white_reward;
                                if ( winner == 0 ) {
                                        black_reward = 0.f;
                                } else if ( winner == BLACK ) {
                                        black_reward = 1.f;
                                } else {
                                        black_reward = -1.f;
                                }
                                white_reward = -1 * black_reward;
                                mcts_backup_rewards(
                                    black_reward, white_reward, simulate_len,
                                    simulate_path_node, simulate_path_col );
                                game_free( dup_game );
                                break; /* End of this iteration. */
                        }

                        /* Expand new leaf */
                        if ( node->c[col] == NULL ) {
                                MCTSNode *expanded_node = mcts_node_new(
                                    /*moved_in*/ dup_game, node->nn );
                                node->c[col] =
                                    expanded_node; /* owned by node */
                                f32 black_reward;
                                f32 white_reward;
                                black_reward = expanded_node->predicated_reward;
                                if ( expanded_node->game_snapshot
                                         ->next_player == WHITE )
                                        black_reward *= -1.f;
                                white_reward = -1 * black_reward;
                                mcts_backup_rewards(
                                    black_reward, white_reward, simulate_len,
                                    simulate_path_node, simulate_path_col );
                                break; /* End of this iteration. */
                        }

                        game_free( dup_game );
                        /* Keeps playing in this iteration. */
                        node = node->c[col];
                }

                /* Report the progress.
                 *
                 * To avoid over-spamming, the condition is
                 * - First iteration
                 * - Last iteration
                 * - at least 2 seconds have passed.
                 */
                time_t now = time( NULL );
                if ( it == 0 || now - last_report_progress >= 2 ||
                     it == iterations - 1 ) {
                        last_report_progress = now;
                        float progress =
                            (f32)( it + 1 ) / (f32)iterations * 100.f;
                        printf( "Progress: [%5.1f%%]\n", progress );
                }
        }
}

/* === Game related utilities ----------------------------------------------- */

/* Creates a new game and initializes nn player randomly. */
Game *
game_new( void )
{
        Game *g = calloc( 1, sizeof( *g ) );
        assert( g != NULL );
        g->next_player = BLACK;
        g->nn_player   = ( ( rand( ) % 2 == 0 ) ? BLACK : WHITE );
        return g;
}

void
game_free( Game *g )
{
        if ( g == NULL ) return;
        free( g );
}

Game *
game_dup_snapshot( Game *g )
{
        Game *ng = malloc( sizeof( *ng ) );
        assert( ng != NULL );
        memcpy( ng, g, sizeof( *ng ) );
        return ng;
}

/* Display the board. */
void
show_board( Game *g )
{
        printf( "  " );
        for ( int i = 0; i < COLS; i++ ) printf( " %d", i + 1 );
        printf( "\n" );

        for ( int y = 0; y < ROWS; y++ ) {
                printf( "%d ", y + 1 );
                for ( int x = 0; x < COLS; x++ ) {
                        Color c = g->board[COL_ROW_TO_IDX( x, y )];
                        if ( c == NA )
                                printf( " ." );
                        else if ( c == BLACK )
                                printf( " x" );
                        else if ( c == WHITE )
                                printf( " o" );
                }
                printf( "\n" );
        }
}

/* Return the next legal row to place a stone in column (col) or -1 if no way.
 */
int
game_legal_row( Game *g, int col )
{
        Color *board = g->board;
        for ( int row = ROWS - 1; row >= 0; row-- ) {
                if ( board[COL_ROW_TO_IDX( col, row )] == NA ) return row;
        }
        return -1;
}

/* Return BLACK or WHITE if winner exists, 0 if tie, -1 if game is still
 * ongoing.
 */
int
game_winner( Game *g )
{
        Color *board = g->board;
        assert( (int)BLACK > 0 );
        assert( (int)WHITE > 0 );
        /* Pass 1: check winner. */
        for ( int row = 0; row < ROWS; row++ ) {
                for ( int col = 0; col < COLS; col++ ) {
                        Color c = board[COL_ROW_TO_IDX( col, row )];
                        if ( c == NA ) continue;

                        /* Go right */
                        {
                                int i = 1;
                                for ( ; i <= 3; i++ ) {
                                        if ( col + i >= COLS ) break;
                                        int idx =
                                            COL_ROW_TO_IDX( col + i, row );
                                        if ( board[idx] != c ) break;
                                }

                                if ( i == 4 ) return (int)c;
                        }

                        /* Go down */
                        {
                                int i = 1;
                                for ( ; i <= 3; i++ ) {
                                        if ( row + i >= ROWS ) break;
                                        int idx =
                                            COL_ROW_TO_IDX( col, row + i );
                                        if ( board[idx] != c ) break;
                                }

                                if ( i == 4 ) return (int)c;
                        }

                        /* Go up right */
                        {
                                int i = 1;
                                for ( ; i <= 3; i++ ) {
                                        if ( row - i < 0 ) break;
                                        if ( col + i >= COLS ) break;
                                        int idx =
                                            COL_ROW_TO_IDX( col + i, row - i );
                                        if ( board[idx] != c ) break;
                                }

                                if ( i == 4 ) return (int)c;
                        }

                        /* Go down right */
                        {
                                int i = 1;
                                for ( ; i <= 3; i++ ) {
                                        if ( col + i >= COLS ) break;
                                        if ( row + i >= ROWS ) break;
                                        int idx =
                                            COL_ROW_TO_IDX( col + i, row + i );
                                        if ( board[idx] != c ) break;
                                }

                                if ( i == 4 ) return (int)c;
                        }
                }
        }

        /* Pass 2: check tie. */
        int tie = 1;
        for ( int x = 0; x < ROWS * COLS; x++ ) {
                if ( board[x] == NA ) {
                        tie = 0;
                        break;
                }
        }

        if ( tie ) return 0;

        /* Still ongoing */
        return -1;
}

int
policy_random_move( Game *g )
{
        while ( 1 ) {
                int col = rand( ) % COLS;
                int row = game_legal_row( g, col );
                if ( row != -1 ) return col;
        }
}

int
policy_human_move( Game *g )
{
        while ( 1 ) {
                int  col;
                char movec;
                printf( "[%s] Your move (1-7): ",
                        g->nn_player == BLACK ? "o" : "x" );
                if ( EOF == scanf( " %c", &movec ) ) {
                        PANIC( "eof, unexpected.\n" );
                }
                col = movec - '1';  // Turn character into number.

                if ( col < 0 || col >= COLS ) {
                        printf(
                            "Invalid move! Must be ('1'-'7'). Try again.\n" );
                        continue;
                }

                int row = game_legal_row( g, col );
                if ( row == -1 ) {
                        printf( "Invalid move! Column is full. Try again.\n" );
                        continue;
                }
                return col;
        }
}

int
policy_nn_move( Game *g, NN *nn )
{
        Tensor *in;
        convert_game_to_tensor_input( &in, g );

        /* Call nn
         *
         * The output specification:
         *
         * - The output is a single tensor with ROWS*COLS value. Each is a
         *   probability of the position to place next stone (even the position
         *   is not legal move)
         * - The index of each probability is same as COL_ROW_TO_IDX. Top left
         *   corner is (col=0,row=0).
         */
        Tensor *out; /* Policy output. */
        Tensor *value_out;
        nn_forward( nn, in, &out, &value_out );
        RESET_TENSOR( value_out );
        RESET_TENSOR( in );

        assert( out->ele_total == ROWS * COLS );

        f32 best     = -100000000000.f;
        int best_col = -1;
        for ( int col = 0; col < COLS; col++ ) {
                int row = game_legal_row( g, col );
                if ( row == -1 ) continue;
                int idx = COL_ROW_TO_IDX( col, row );
                f32 v   = out->data[idx];
                if ( best_col == -1 || v > best ) {
                        best     = v;
                        best_col = col;
                }
        }
        if ( best_col == -1 ) {
                PANIC( "is the board full???" );
        }

        RESET_TENSOR( out );
        return best_col;
}

int
policy_nn_mcts_move( Game *g, NN *nn )
{
        Game     *dup_game = game_dup_snapshot( g );
        MCTSNode *root     = mcts_node_new( /*moved_in*/ dup_game, nn );
        mcts_run_simulation( root, MCTS_ITER_CNT );
        int col = mcts_node_select_next_col_to_play( root );
        mcts_node_free( root );
        return col;
}

void
play_game( NN *nn )
{
        Game *g = game_new( );

        show_board( g );
        while ( 1 ) {
                int col, row;

                if ( g->next_player == g->nn_player ) {
                        // col = policy_random_move( g );
                        // col = policy_nn_move( g, nn );
                        col = policy_nn_mcts_move( g, nn );
                } else {
#ifdef MCTS_SELF_PLAY
                        col = policy_nn_mcts_move( g, nn );
#else
                        col = policy_human_move( g );
#endif
                }

                if ( col < 0 || col >= COLS ) {
                        printf( "invalid column during game %d\n", col );
                        PANIC( "unexpected" );
                }

                row = game_legal_row( g, col );
                if ( row == -1 ) {
                        printf( "invalid column during game %d\n", col );
                        PANIC( "sorry" );
                }

                printf( "Place new stone in column: %d\n", col + 1 );
                g->board[COL_ROW_TO_IDX( col, row )] = g->next_player;
                show_board( g );

                int winner = game_winner( g );
                switch ( winner ) {
                case (int)BLACK:
                        printf( "black wins\n" );
                        goto cleanup;
                case (int)WHITE:
                        printf( "white wins\n" );
                        goto cleanup;
                case 0:
                        printf( "tie\n" );
                        goto cleanup;
                default:
                        g->next_player =
                            g->next_player == BLACK ? WHITE : BLACK;
                }
        }
cleanup:
        game_free( g );
}

/* === Main ----------------------------------------------------------------- */

int
main( void )
{
        srand( (unsigned)time( NULL ) );
        NN *nn = nn_new( BIN_DATA_FILE );
        play_game( nn );
        nn_free( nn );
}
