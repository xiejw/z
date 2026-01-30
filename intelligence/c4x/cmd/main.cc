#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <time.h>

#include "game.h"
#include "log.h"
#include "mcts.h"
#include "nn.h"
#include "tensor.h"

using namespace hermes;

/* === --- Configurations and Macros ------------------------------------ === */

#define BIN_DATA_FILE ".build/tensor_data.bin" /* Tensor data dump file */

// Simulation iteration count.
//
// References:
// - 1600 is quite strong but requires tons of compute. I never win.
// -  400 might be reasonably good to play but faster.
//
#ifndef MCTS_ITER_CNT
#define MCTS_ITER_CNT 1600
#endif

/* === --- Policy ------------------------------------------------------- === */

int
policy_random_move( Game *g )
{
        while ( 1 ) {
                int col = rand( ) % COLS;
                int row = game_legal_row( g, col );
                if ( row != -1 ) return col;
        }
}

int
policy_human_move( Game *g )
{
        while ( 1 ) {
                int  col;
                char movec;
                printf( "[%s] Your move (1-7): ",
                        g->nn_player == BLACK ? "o" : "x" );
                if ( EOF == scanf( " %c", &movec ) ) {
                        PANIC( "eof, unexpected.\n" );
                }
                col = movec - '1';  // Turn character into number.

                if ( col < 0 || col >= COLS ) {
                        printf(
                            "Invalid move! Must be ('1'-'7'). Try again.\n" );
                        continue;
                }

                int row = game_legal_row( g, col );
                if ( row == -1 ) {
                        printf( "Invalid move! Column is full. Try again.\n" );
                        continue;
                }
                return col;
        }
}

int
policy_nn_move( Game *g, NN *nn )
{
        Tensor *in;
        Valconvert_game_to_tensor_input( &in, g );

        // Call nn
        //
        // The output specification:
        //
        // - The output is a single tensor with ROWS*COLS value. Each is a
        //   probability of the position to place next stone (even the position
        //   is not legal move)
        // - The index of each probability is same as COL_ROW_TO_IDX. Top left
        //   corner is (col=0,row=0).
        //
        Tensor *out;        // Policy output.
        Tensor *value_out;  // Value output, not used.
        nn_forward( nn, in, &out, &value_out );
        free_tensor( value_out );
        free_tensor( in );

        assert( out->ele_total == ROWS * COLS );

        f32 best     = -100000000000.f;
        int best_col = -1;
        for ( int col = 0; col < COLS; col++ ) {
                int row = game_legal_row( g, col );
                if ( row == -1 ) continue;
                int idx = COL_ROW_TO_IDX( col, row );
                f32 v   = out->data[idx];
                if ( best_col == -1 || v > best ) {
                        best     = v;
                        best_col = col;
                }
        }
        if ( best_col == -1 ) {
                PANIC( "is the board full???" );
        }

        free_tensor( out );
        return best_col;
}

int
policy_nn_mcts_move( Game *g, NN *nn )
{
        Game     *dup_game = game_dup_snapshot( g );
        MCTSNode *root     = mcts_node_new( /*moved_in*/ dup_game, nn );
        mcts_run_simulation( root, MCTS_ITER_CNT );
        int col = mcts_node_select_next_col_to_play( root );
        mcts_node_free( root );
        return col;
}

/* === --- Play the Game ------------------------------------------------ === */

void
play_game( NN *nn )
{
        Game *g = game_new( );

        show_board( g );
        while ( 1 ) {
                int col, row;

                if ( g->next_player == g->nn_player ) {
                        col = policy_nn_mcts_move( g, nn );
                } else {
#ifdef MCTS_SELF_PLAY
                        col = policy_nn_mcts_move( g, nn );
#else
                        col = policy_human_move( g );
#endif
                }

                if ( col < 0 || col >= COLS ) {
                        printf( "invalid column during game %d\n", col );
                        PANIC( "unexpected" );
                }

                row = game_legal_row( g, col );
                if ( row == -1 ) {
                        printf( "invalid column during game %d\n", col );
                        PANIC( "sorry" );
                }

                printf( "Place new stone in column: %d\n", col + 1 );
                g->board[COL_ROW_TO_IDX( col, row )] = g->next_player;
                show_board( g );

                int winner = game_winner( g );
                switch ( winner ) {
                case (int)BLACK:
                        printf( "black wins\n" );
                        goto cleanup;
                case (int)WHITE:
                        printf( "white wins\n" );
                        goto cleanup;
                case 0:
                        printf( "tie\n" );
                        goto cleanup;
                default:
                        g->next_player =
                            g->next_player == BLACK ? WHITE : BLACK;
                }
        }
cleanup:
        game_free( g );
}

/* === --- Main --------------------------------------------------------- === */

int
main( void )
{
        srand( (unsigned)time( NULL ) );
        NN *nn = nn_new( /*data_file=*/BIN_DATA_FILE );
        play_game( nn );
        nn_free( nn );
}
