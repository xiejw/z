#include <assert.h>
#include <fcntl.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BIN_DATA_FILE    "tensor_data.bin" /* Tensor data dump file */
#define MAX_DIM_LIMIT    5                 /* Max dim for tenshor shape. */
#define MAX_TENSOR_LIMIT 128               /* Max number of tensors. */
#define MAX_ELE_DISPLAY  20    /* Max number of elements to display. */
#define REL_ERROR        1e-2f /* Max relative error allowed during assertion. */
#define BN_EPS           0.001f /* Eps for Batch norm. */

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
typedef uint32_t u32;
typedef int32_t  i32;

typedef struct {
        u32  dim;
        u32  shape[MAX_DIM_LIMIT];
        u32  ele_total;
        f32 *data;
} Tensor;

typedef struct {
        u32    weight_cnt;                /* Total number of weights. */
        Tensor weights[MAX_TENSOR_LIMIT]; /* All weights. */
        // u32    weight_idx; /* the index to read the next weight. */
} NN;

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

/* Free tensor data inside a static allocated tensor arary (tensors). */
void
free_static_tensor_data( u32 tensor_cnt, Tensor *tensors )
{
        for ( u32 i = 0; i < tensor_cnt; i++ ) {
                free( tensors[i].data );
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
 */

// clang-format off
/*
+ ResNetModelWrapper(
+  (conv2d): Conv2d(3, 128, kernel_size=(5, 5), stride=(1, 1), padding=same)
+  (batch_n): BatchNorm2d(128, eps=0.001, momentum=0.1, affine=True, track_running_stats=True)
+  (relu): ReLU(inplace=True)
+  (b0_conv2d): Conv2d(128, 128, kernel_size=(5, 5), stride=(1, 1), padding=same)
+  (b0_batch_n): BatchNorm2d(128, eps=0.001, momentum=0.1, affine=True, track_running_stats=True)
+  (b0_relu): ReLU(inplace=True)
+  (b0_conv2d_b): Conv2d(128, 128, kernel_size=(5, 5), stride=(1, 1), padding=same)
+  (b0_batch_n_b): BatchNorm2d(128, eps=0.001, momentum=0.1, affine=True, track_running_stats=True)
+  (b0_add): ResidualAddLayer()
+  (b0_relu_b): ReLU(inplace=True)
+  (b1_conv2d): Conv2d(128, 128, kernel_size=(5, 5), stride=(1, 1), padding=same)
+  (b1_batch_n): BatchNorm2d(128, eps=0.001, momentum=0.1, affine=True, track_running_stats=True)
+  (b1_relu): ReLU(inplace=True)
+  (b1_conv2d_b): Conv2d(128, 128, kernel_size=(5, 5), stride=(1, 1), padding=same)
+  (b1_batch_n_b): BatchNorm2d(128, eps=0.001, momentum=0.1, affine=True, track_running_stats=True)
+  (b1_add): ResidualAddLayer()
+  (b1_relu_b): ReLU(inplace=True)
+  (b2_conv2d): Conv2d(128, 128, kernel_size=(5, 5), stride=(1, 1), padding=same)
+  (b2_batch_n): BatchNorm2d(128, eps=0.001, momentum=0.1, affine=True, track_running_stats=True)
+  (b2_relu): ReLU(inplace=True)
+  (b2_conv2d_b): Conv2d(128, 128, kernel_size=(5, 5), stride=(1, 1), padding=same)
+  (b2_batch_n_b): BatchNorm2d(128, eps=0.001, momentum=0.1, affine=True, track_running_stats=True)
+  (b2_add): ResidualAddLayer()
+  (b2_relu_b): ReLU(inplace=True)
+  (b3_conv2d): Conv2d(128, 128, kernel_size=(5, 5), stride=(1, 1), padding=same)
+  (b3_batch_n): BatchNorm2d(128, eps=0.001, momentum=0.1, affine=True, track_running_stats=True)
+  (b3_relu): ReLU(inplace=True)
+  (b3_conv2d_b): Conv2d(128, 128, kernel_size=(5, 5), stride=(1, 1), padding=same)
+  (b3_batch_n_b): BatchNorm2d(128, eps=0.001, momentum=0.1, affine=True, track_running_stats=True)
+  (b3_add): ResidualAddLayer()
+  (b3_relu_b): ReLU(inplace=True)
+  (b4_conv2d): Conv2d(128, 128, kernel_size=(5, 5), stride=(1, 1), padding=same)
+  (b4_batch_n): BatchNorm2d(128, eps=0.001, momentum=0.1, affine=True, track_running_stats=True)
+  (b4_relu): ReLU(inplace=True)
+  (b4_conv2d_b): Conv2d(128, 128, kernel_size=(5, 5), stride=(1, 1), padding=same)
+  (b4_batch_n_b): BatchNorm2d(128, eps=0.001, momentum=0.1, affine=True, track_running_stats=True)
+  (b4_add): ResidualAddLayer()
+  (b4_relu_b): ReLU(inplace=True)
+  (p_conv2d): Conv2d(128, 2, kernel_size=(1, 1), stride=(1, 1), padding=same)
+  (p_batch_n): BatchNorm2d(2, eps=0.001, momentum=0.1, affine=True, track_running_stats=True)
+  (p_relu): ReLU(inplace=True)
+  (p_flatten): Flatten(start_dim=1, end_dim=-1)
+  (p_dense): Linear(in_features=84, out_features=42, bias=True)
+  (p_softmax): Softmax(dim=1)
+  (v_conv2d): Conv2d(128, 2, kernel_size=(1, 1), stride=(1, 1), padding=same)
+  (v_batch_n): BatchNorm2d(2, eps=0.001, momentum=0.1, affine=True, track_running_stats=True)
+  (v_relu): ReLU(inplace=True)
+  (v_flatten): Flatten(start_dim=1, end_dim=-1)
+  (v_dense_1): Linear(in_features=84, out_features=256, bias=True)
+  (v_dense_2): Linear(in_features=256, out_features=1, bias=True)
+  (v_tanh): Tanh()
+)
 */
// clang-format on

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

/* Add is the Residul add in the resnet block. For performance, this layer does
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

/* Linear layer to perform matmul on (1, C) x (C, N) = (1, N).
 *
 * NOTE
 * - Batch size is assumed to be 1.
 * - The weight matrix is assumed to have shape (N, C) rather than (C, N)
 * - Input is ok to have more than 2 dim, and we implicitly do a flatten.
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

        *dst = output;
}

NN *
nn_new( const char *data_file )
{
        NN *nn = malloc( sizeof( *nn ) );
        assert( nn != NULL );
        nn->weight_cnt = 0;
        // TODO nn->weight_idx = 0;
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
nn_forward( NN *nn )
{
        Tensor *input  = NULL;
        Tensor *output = NULL;

        u32 weight_idx = 0;

        /* Layer 0 */
        conv2d( &output, &nn->weights[weight_idx + 0],
                &nn->weights[weight_idx + 1], &nn->weights[weight_idx + 2] );
        weight_idx += 3;

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
        input = output;
        policy_head( &output, input, &weight_idx, nn->weights );

        /* Debug the output of the final layer. */
        printf( "assert outputs with weight_idx %u\n", weight_idx );
        show_tensor( output, "output" );
        show_tensor( &nn->weights[weight_idx], "target" );
        assert_tensors_equal( output, &nn->weights[weight_idx] );
        weight_idx++;

        /* Output tensor must be the final one. */
        assert( weight_idx == nn->weight_cnt );

        RESET_TENSOR( input );
        RESET_TENSOR( output );
}
/* === Main ----------------------------------------------------------------- */

int
main( void )
{
        NN *nn = nn_new( BIN_DATA_FILE );
        nn_forward( nn );
        nn_free( nn );
}
