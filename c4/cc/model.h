#ifndef C4_MODEL_H_
#define C4_MODEL_H_

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
#define BOARD_SIZE 42
#define ROW_COUNT  6
#define COL_COUNT  7

//
// apis
//
error_t c4_model_predict(
    /* inputs */
    const color_t next_player_color, const color_t *board, const int pos_count,

    /* outputs */
    f32_t **prob, f32_t *value);

void c4_model_cleanup();

#endif  // C4_MODEL_H_
