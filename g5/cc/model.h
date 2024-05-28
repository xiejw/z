#pragma once

#include <torch/torch.h>

#include "base.h"

//
// color
//
typedef int color_t;

#define BLACK_INT 1
#define WHITE_INT -1
#define NA_INT    0

//
// board
//
#define BOARD_SIZE ( 15 * 15 )
#define ROW_COUNT  15
#define COL_COUNT  15

//
// apis
//
error_t c4_model_predict(
    /* inputs */
    const color_t next_player_color, const color_t *board, const int pos_count,

    /* outputs */
    f32_t **prob, f32_t *value );

error_t call_model( torch::jit::script::Module &module, torch::Tensor &input,
                    f32_t **prob, f32_t *value );
