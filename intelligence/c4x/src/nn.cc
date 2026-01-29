#include "nn.h"

#include "log.h"

/* === BLAS related header and kernels -------------------------------------- */

#ifdef MACOS_ACCELERATE
#include <Accelerate/Accelerate.h>
#endif

#ifdef BLAS
#include <cblas.h>
#endif

#define BN_EPS 0.001f /* EPS for Batch norm. */

#define RESET_TENSOR( t )         \
        do {                      \
                free_tensor( t ); \
                ( t ) = NULL;     \
        } while ( 0 )

#define DEBUG \
        if ( !DISABLE_SHOW_TENSOR ) INFO

/* === NN -------------------------------------------------------------------
 *
 * See reference_model.py for the PyTorch reference model.
 */

namespace hermes {

namespace {
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
        t->data           = (f32 *)malloc( byte_count );
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

}  // namespace
NN *
nn_new( const char *data_file )
{
        NN *nn = (NN *)malloc( sizeof( *nn ) );
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

}  // namespace hermes

namespace hermes {
namespace {
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
}  // namespace
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

        u32 shape[] = { 1, c_out, h, w };
        alloc_tensor( dst, 4, shape );
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
namespace {

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
}  // namespace

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

        u32 shape[] = { 1, c_out, h, w };
        alloc_tensor( dst, 4, shape );
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
        u32     shape1[] = { h * w, kernel_h * kernel_w * c_in };
        alloc_tensor( &col_matrix, 2, shape1 );

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

void
relu_inplace( Tensor *t )
{
        u32 size = t->ele_total;
        for ( u32 i = 0; i < size; i++ ) {
                f32 f = t->data[i];
                if ( f < 0 ) t->data[i] = 0.f;
        }
}

void
add_inplace( Tensor *dst, Tensor *src )
{
        assert( dst->ele_total == src->ele_total );
        u32 size = dst->ele_total;
        for ( u32 i = 0; i < size; i++ ) {
                dst->data[i] += src->data[i];
        }
}

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

        u32 shape[] = { 1, out_dim };
        alloc_tensor( dst, 2, shape );
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

}  // namespace hermes
