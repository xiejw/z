## c4c

A pure c implementation with zero dependency to play connect 4.  The model is
trained in Python world by self-playing the game with MCTS.

### Get Started

To play
```
make RELEASE=1
make                                               # Debug mode
make RELEASE=1 MCTS_ITER_CNT=1600                  # Really strong nn player but slow
make RELEASE=1 MCTS_ITER_CNT=600                   # Strong nn player but faster
make RELEASE=1 MCTS_ITER_CNT=1600 MCTS_SELF_PLAY=1 # Two nn players play each other

# If you have openblas installed on Linux, try this
make RELEASE=1 BLAS=1
```
Have fun!

### Performance and BLAS

With few days of coding, the performance is reasonably OK if MCTS is not used.
I did not do any optimization (fuse, threading, etc), the c code is quite fast.

However, MCTS agent calls `conv2d` too many times, which becomes the bottleneck.
This is expected as `conv2d` and `matmul` are two of the dominating layers in
deep learning. This is a sample of the profiling
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

The approach to speed up `conv2d` is using the trick `im2col + matmul`, where
`matmul` can leverage highly optimized BLAS `cblas_sgemm` library call. This
gives 50x speed up on macOs for my local testing which provides BLAS via
`accelerate` framework (this is enabled by default if macOs is detected).

On Linux, I have tested `openblas` as follows
```
# Debian
sudo apt install libopenblas-dev
make RELEASE=1 BLAS=1

# ARCH
sudo pacman -S blas-openblas
CFLAGS=-I/usr/include/openblas make RELEASE=1 BLAS=1
```
