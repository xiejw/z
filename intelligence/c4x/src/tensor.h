// vim: ft=cpp
// forge:v1
// hermes:v1
#pragma once

#include <stdint.h>
#include <stdio.h>

typedef float    f32;
typedef uint32_t u32;
typedef int32_t  i32;

#define DISABLE_SHOW_TENSOR 1

#define MAX_DIM_LIMIT 4 /* Max dim for tensor shape. */

/* === --- Tensor Related Data Structures ------------------------------- === */

namespace hermes {

typedef struct {
        u32  dim;
        u32  shape[MAX_DIM_LIMIT];
        u32  ele_total;
        f32 *data;
} Tensor;

/* Display tensor elements in stdout after prompt. Number of elements to be
 * displayed is limited by MAX_ELE_DISPLAY.
 */
void show_tensor( Tensor *t, const char *prompt );

/* Allocates a tensor with shape and dim but random data buffer. */
void alloc_tensor( Tensor **dst, u32 dim, u32 *shape );

/* Dup a tensor with the same shape and dim as src. If copy_data is non-zero,
 * the data buffer is copied as well.
 */
void dup_tensor( Tensor **dst, Tensor *src, int copy_data );

/* Free a tensor on heap. */
void free_tensor( Tensor *p );

/* Free tensor data inside a static allocated tensor array (tensors). */
void free_static_tensor_data( u32 tensor_cnt, Tensor *tensors );
}  // namespace hermes
