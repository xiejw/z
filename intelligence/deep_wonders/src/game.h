// vim: ft=cpp
// deep_wonders:v1
#pragma once

#define NUM_ACTIONS 3 /* Stub: 3 placeholder actions */
#define MAX_TURNS  10 /* Stub: game ends after 10 turns */

namespace deep_wonders {

typedef struct {
        int current_player; /* 0 or 1 */
        int turn;           /* Current turn number, starts at 0. */
        int done;           /* 1 if game is over. */
        int winner;         /* -1 ongoing, 0/1 winner, 2 tie. */
} Game;

/// Create a new game in its initial state.
Game *game_new( void );

/// Free a game.
void game_free( Game *g );

/// Duplicate a game state (deep copy).
Game *game_dup( Game *g );

/// Return the number of legal actions (stub: always NUM_ACTIONS).
int game_num_actions( Game *g );

/// Apply an action to the game, advancing the state.
void game_apply_action( Game *g, int action );

/// Return the winner: -1 ongoing, 0/1 winner, 2 tie.
int game_winner( Game *g );

/// Return 1 if the game is over, 0 otherwise.
int game_is_over( Game *g );

/// Print the current game state.
void show_game( Game *g );

}  // namespace deep_wonders
