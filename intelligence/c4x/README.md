## c4x

This is a standalone C++-17 implementation of Connect Four, requiring no
external dependencies. The model was trained in Python using self-play guided by
Monte Carlo Tree Search (MCTS).

When a `BLAS` library is available—such as the `Accelerate` framework on
macOS—the implementation automatically leverages it to improve performance.

### Get Started

To play
```
make RELEASE=1

# If you have BLAS library, try this knob BLAS=1.
#
# For example,
# - Linux: OpenBLAS
# - macOS: BLAS is enabled by default.
#
make RELEASE=1 BLAS=1

# Advanced knobs
make                                               # Debug mode
make RELEASE=1 MCTS_ITER_CNT=1600                  # Really strong nn player but slow
make RELEASE=1 MCTS_ITER_CNT=400                   # Strong nn player but faster
make RELEASE=1 MCTS_ITER_CNT=1600 MCTS_SELF_PLAY=1 # Two nn players play each other

```
Have fun!

### Performance and BLAS

After a few days of development, the performance is reasonably acceptable when
MCTS is not utilized. No performance optimizations—such as operation fusion,
multi-threading, or SIMD—have been applied. Nevertheless, the C implementation
demonstrates substantial speed and outperforms the original Python and
PyTorch-based version

However, the MCTS agent invokes the `conv2d` operation excessively, resulting in
a performance bottleneck. This behavior is anticipated, as `conv2d` and `matmul`
(matrix multiplication) are among the most computationally intensive Ops in
deep learning models.


This is a sample of the profiling result:
```
Each sample counts as 0.01 seconds.
  %   cumulative   self              self     total
 time   seconds   seconds    calls   s/call   s/call  name
 99.96     27.85    27.85     3081     0.01     0.01  conv2d
  0.07     27.87     0.02     1185     0.00     0.02  resnet_block
  0.00     27.87     0.00      453     0.00     0.00  game_winner
  0.00     27.87     0.00      411     0.00     0.00  mcts_node_select_next_col_to_evaluate
  0.00     27.87     0.00      237     0.00     0.00  convert_game_to_tensor_input
  0.00     27.87     0.00      237     0.00     0.12  mcts_node_new
  0.00     27.87     0.00      237     0.00     0.12  nn_forward
...
```

To accelerate the `conv2d` operation, the `im2col + matmul` technique is employed
(refer to this
[page](https://cs231n.github.io/convolutional-networks/#:~:text=Implementation%20as%20Matrix,output%20dimension%20[55x55x96].)
for the algorithm).
The `im2col` transformation incurs minimal
overhead, while the matrix multiplication step (`matmul`) leverages the highly
optimized `BLAS` routine `cblas_sgemm`. In local testing on macOs, this approach
yielded a `50x`–`60x` performance improvement, benefiting from the `Accelerate`
framework, which is enabled by default when macOs is detected.

On Debian/Linux, I have tested `OpenBLAS` as follows
```
# Debian
sudo apt install libopenblas-dev
make RELEASE=1 BLAS=1
```
On other system, once `openblas` or any `BLAS` library is installed, it should
work as follows
```
# macOS with Old SDK
export MACOSX_DEPLOYMENT_TARGET=13.3
make RELEASE=1
```
