// vim: ft=cpp
// forge:v1
// hermes:v1
#pragma once

#include "tensor.h"

#define ROWS 6
#define COLS 7

#define COL_ROW_TO_IDX( col, row ) ( ( row ) * COLS + ( col ) )

namespace hermes {
/* === Game play related data structures ------------------------------------ */

typedef enum { NA, BLACK, WHITE } Color;

typedef struct {
        Color board[ROWS * COLS];
        Color next_player;
        Color nn_player;
} Game;

/* Creates a new game and initializes nn player randomly. */
Game *game_new( void );
void  game_free( Game *g );

Game *game_dup_snapshot( Game *g );

/* Display the board. */
void show_board( Game *g );

/// Return the next legal row to place a stone in column (col) or -1 if no way.
int game_legal_row( Game *g, int col );

/// Return BLACK or WHITE if winner exists, 0 if tie, -1 if game is still
/// ongoing.
int game_winner( Game *g );

/// Convert game board to feature input.
///
/// The input tensor specification:
///
/// - For 3 channel input with shape {1,3,ROWS,COLS}, all f32 data are
///   zero by default.
/// - If the next_player is BLACK, the 3rd channel, i.e., [1,2,:,:] are
///   set to 1.
/// - For each stone on the board, if it is BLACK, the corresponding f32
///   data in the 1st channel is 1, i.e., [1,0,row,col] = 1. If WHITE,
///   the 2nd channel for that data is 1, i.e, [1,1,row,col] = 1.
///
void convert_game_to_tensor_input( Tensor **dst, Game *g );
}  // namespace hermes
