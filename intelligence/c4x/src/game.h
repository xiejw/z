// vim: ft=cpp
// forge:v1
// hermes:v1
#pragma once

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
}  // namespace hermes
