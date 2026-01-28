// vim: ft=cpp
#pragma once

#include "ctx.h"

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
#define BOARD_SIZE    42
#define ROW_COUNT     6
#define COL_COUNT     7
#define CHANNEL_COUNT 3

//
// apis
//
namespace c4 {
auto model_init( ) -> error_t;
void model_deinit( );

auto model_predict( const color_t next_player_color, const color_t *board,
                    const int board_size, f32_t **_C4_Out p_prob,
                    f32_t *_C4_Out p_value ) -> error_t;
}  // namespace c4
