// vim: ft=cpp
// forge:v1
// hermes:v1
#pragma once

#include "tensor.h"

#define MAX_TENSOR_LIMIT 128 /* Max number of tensors. */

#if defined( MACOS_ACCELERATE ) || defined( BLAS )
#define conv2d conv2d_blas
#else
#define conv2d conv2d_naive
#endif

namespace hermes {

/* === --- NN Related Data Structures ----------------------------------- === */
typedef struct {
        u32    weight_cnt;                /* Total number of weights. */
        Tensor weights[MAX_TENSOR_LIMIT]; /* All weights. */
} NN;

NN  *nn_new( const char *data_file );
void nn_free( NN *p );
void nn_forward( NN *nn, Tensor *in, Tensor **policy_out, Tensor **value_out );

/* === --- ML Related Data Structures ----------------------------------- === */
/* Conv2D on a 4D (N, C_in, H, W) input tensor with
 * - (C_out, C_in, KH, KW) * kernel (weight) and
 * - (C_out) bias.
 *
 * NOTE:
 * - This naive implementation assumes batch size is 1, i.e., N=1;
 * - This naive implementation assumes conv2d is same padding, KH and KW are
 *   both odd numbers.
 */
void conv2d_naive( Tensor **dst, Tensor *input, Tensor *weight, Tensor *bias );

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
void conv2d_blas( Tensor **dst, Tensor *input, Tensor *weight, Tensor *bias );

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
void batchnorm2d( Tensor **dst, Tensor *input, Tensor *weight, Tensor *bias,
                  Tensor *mean, Tensor *var );

/* Relu performs max(0, x) for each element x. For performance, this layer does
 * in place update.
 *
 * NOTE: We could fuse this with previous layer to avoid one more mem reading.
 */
void relu_inplace( Tensor *t );

/* Add is the Residual add in the resnet block. For performance, this layer does
 * in place update to the dst.
 *
 * NOTE: We could fuse this with previous layer to avoid one more mem reading.
 */
void add_inplace( Tensor *dst, Tensor *src );

/* Perform softmax on the 1-st dim (0-based). For performance, this layer does
 * in place update.
 *
 * NOTE:
 * - batch size is assume to be 1.
 */
void softmax_inplace( Tensor *dst );

/* Tanh performs element wise tanh op on input. */
void tanh_inplace( Tensor *dst );

/* Linear layer to perform matmul on (1, C) x (C, N) = (1, N).
 *
 * NOTE
 * - Batch size is assumed to be 1.
 * - The weight matrix is assumed to have shape (N, C) rather than (C, N)
 * - Input is OK to have more than 2 dim, and we implicitly do a flatten.
 */
void linear( Tensor **dst, Tensor *input, Tensor *weight, Tensor *bias );

void resnet_block( Tensor **dst, Tensor *src, u32 *weight_idx,
                   Tensor *weights );
void policy_head( Tensor **dst, Tensor *src, u32 *weight_idx, Tensor *weights );
void value_head( Tensor **dst, Tensor *src, u32 *weight_idx, Tensor *weights );
}  // namespace hermes
