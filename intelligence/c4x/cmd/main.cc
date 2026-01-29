#include <assert.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "nn.h"
#include "tensor.h"

using namespace hermes;

/* === Configurations and Macros -------------------------------------------- */

#define ROWS 6
#define COLS 7

#define BIN_DATA_FILE ".build/tensor_data.bin" /* Tensor data dump file */
#define MCTS_PROB_LOW_LIMIT \
        0.05f /* The low limit we allow for each MCTS node. */

#ifndef MCTS_ITER_CNT
/* Simulation iteration count.
 * - 1600 is quite strong but requires tons of compute. I never win.
 * - 400 might be reasonably good to play but faster.
 */
#define MCTS_ITER_CNT 1600
#endif

#define PANIC( msg )                 \
        do {                         \
                printf( "panic\n" ); \
                printf( msg );       \
                exit( -1 );          \
        } while ( 0 )

#define COL_ROW_TO_IDX( col, row ) ( ( row ) * COLS + ( col ) )

#define RESET_TENSOR( t )         \
        do {                      \
                free_tensor( t ); \
                ( t ) = NULL;     \
        } while ( 0 )

/* === Game play related data structures ------------------------------------ */

typedef enum { NA, BLACK, WHITE } Color;

typedef struct {
        Color board[ROWS * COLS];
        Color next_player;
        Color nn_player;
} Game;

/* Convert game board to feature input.
 *
 * The input tensor specification:
 *
 * - For 3 channel input with shape {1,3,ROWS,COLS}, all f32 data are
 *   zero by default.
 * - If the next_player is BLACK, the 3rd channel, i.e., [1,2,:,:] are
 *   set to 1.
 * - For each stone on the board, if it is BLACK, the corresponding f32
 *   data in the 1st channel is 1, i.e., [1,0,row,col] = 1. If WHITE,
 *   the 2nd channel for that data is 1, i.e, [1,1,row,col] = 1.
 */
void
convert_game_to_tensor_input( Tensor **dst, Game *g )
{
        Tensor *in;
        u32     shape[] = { 1, 3, ROWS, COLS };
        alloc_tensor( &in, 4, shape );
        memset( in->data, 0, sizeof( f32 ) * 3 * ROWS * COLS );

        if ( g->next_player == BLACK ) {
                f32 *ptr = in->data + 2 * ROWS * COLS;
                for ( int i = 0; i < ROWS * COLS; i++ ) ptr[i] = 1.0f;
        }

        Color *board     = g->board;
        f32   *black_ptr = in->data;
        f32   *white_ptr = in->data + 1 * ROWS * COLS;
        for ( int row = 0; row < ROWS; row++ ) {
                for ( int col = 0; col < COLS; col++ ) {
                        int   idx = COL_ROW_TO_IDX( col, row );
                        Color c   = board[idx];
                        if ( c == NA ) continue;
                        if ( c == BLACK ) {
                                black_ptr[idx] = 1.0f;
                        } else {
                                white_ptr[idx] = 1.0f;
                        }
                }
        }
        *dst = in;
}

/* === MCTS node and tree --------------------------------------------------- */

Game *game_dup_snapshot( Game *g );
int   game_legal_row( Game *g, int col );
int   game_winner( Game *g );
void  game_free( Game *g );

/* The node data structure for MCTS tree. All simulation rewards information is
 * backed up and recorded in the node.
 *
 * - During evaluation, multi-armed bandit is leveraged to select the next
 *   state to try.
 * - During inference (play), the state with strongest chance is selected.
 */
typedef struct MCTSNode {
        Game *game_snapshot; /* Owned */
        NN   *nn;            /* Unowned */

        /* Total number of visits during multi-armed bandit. */
        int total_count;

        /* The chance current player will win, between -1 and 1. */
        f32 predicated_reward;

        /* Owned children for each legal move. NULL means unexpanded yet. */
        struct MCTSNode *c[COLS];

        /* Visited count for each legal move. -1 for illegal column. */
        int n[COLS];
        /* Backed up total value for each legal move. */
        f32 w[COLS];
        /* Prior probability for each legal move. */
        f32 p[COLS];
} MCTSNode;

/* During creating, NN is invoked to provide predicated_reward (chance to win)
 * and prior probabilities for all legal moves.
 */
MCTSNode *
mcts_node_new( /*moved_in*/ Game *game_snapshot, NN *nn )
{
        MCTSNode *node = (MCTSNode *)calloc( 1, sizeof( *node ) );
        assert( node != NULL );
        node->game_snapshot = game_snapshot; /* owned now */
        node->nn            = nn;

        Tensor *in;
        Tensor *policy_out;
        Tensor *value_out;
        convert_game_to_tensor_input( &in, game_snapshot );
        nn_forward( nn, in, &policy_out, &value_out );

        /* Fill predicated_reward from the value header output. */
        node->predicated_reward = value_out->data[0];

        /* Fill prior probabilities from the policy header output. */
        for ( int col = 0; col < COLS; col++ ) {
                int row = game_legal_row( game_snapshot, col );
                if ( row == -1 ) { /* illegal column. */
                        node->n[col] = -1;
                        continue;
                }
                /* The NN might predict the probability as low as 0.f even it
                 * is a winning move. Force mcts to consider low probability
                 * node.
                 *
                 * NOTE: I did not tune this number well for different
                 * simulation count MCTS_ITER_CNT. */
                f32 p = policy_out->data[COL_ROW_TO_IDX( col, row )];
                if ( p < MCTS_PROB_LOW_LIMIT ) p = MCTS_PROB_LOW_LIMIT;
                node->p[col] = p;
        }
        RESET_TENSOR( in );
        RESET_TENSOR( policy_out );
        RESET_TENSOR( value_out );
        return node;
}

/* Recursively free the entire MCTS tree rooted at n. */
void
mcts_node_free( MCTSNode *n )
{
        if ( n == NULL ) return;
        game_free( n->game_snapshot );
        for ( int i = 0; i < COLS; i++ ) {
                mcts_node_free( n->c[i] );
        }
        free( n );
}

/* Select the next column to evaluate during simulation.
 * Read AlphaGoZero paper (2017, "Methods" section, "Select" paragraph) for
 * details.
 */
int
mcts_node_select_next_col_to_evaluate( MCTSNode *node )
{
        const f32 c                = 1.0f;
        const f32 sqrt_total_count = sqrtf( (f32)node->total_count );
        int       col_to_evaluate  = -1;
        f32       best_q           = 0;
        for ( int col = 0; col < COLS; col++ ) {
                int n = node->n[col];
                if ( n == -1 ) continue; /* illegal col. */
                f32 q = node->w[col] / ( n > 0 ? (f32)n : 1.f );
                q += c * node->p[col] * sqrt_total_count / ( 1.0f + (f32)n );

                if ( col_to_evaluate == -1 || q > best_q ) {
                        col_to_evaluate = col;
                        best_q          = q;
                }
        }
        assert( col_to_evaluate != -1 );
        return col_to_evaluate;
}

/* Select the next column to play. Currently, choose the one with most visited
 * count.
 */
int
mcts_node_select_next_col_to_play( MCTSNode *node )
{
        int col_to_evaluate = -1;
        int best_n          = 0;
        for ( int col = 0; col < COLS; col++ ) {
                int n = node->n[col];
                if ( n == -1 ) continue; /* illegal col. */

                // DEBUG info
                printf( "col %d n %4d p %7.3f avg(w) %7.3f \n", col + 1, n,
                        node->p[col], node->w[col] / (f32)( n > 0 ? n : 1 ) );
                if ( col_to_evaluate == -1 || n > best_n ) {
                        col_to_evaluate = col;
                        best_n          = n;
                }
        }
        assert( col_to_evaluate != -1 );
        return col_to_evaluate;
}

void
mcts_node_backup_reward( MCTSNode *n, int col, f32 reward )
{
        assert( n->n[col] != -1 );
        n->total_count++;
        n->n[col] += 1;
        n->w[col] += reward;
}

#define MAX_MCTS_SIMULATE_PATH_LEN ( ROWS * COLS )

void
mcts_backup_rewards( f32 black_reward, f32 white_reward, int count,
                     MCTSNode **simulate_path_node, int *simulate_path_col )
{
        for ( int i = 0; i < count; i++ ) {
                MCTSNode *n = simulate_path_node[i];
                if ( n->game_snapshot->next_player == BLACK ) {
                        mcts_node_backup_reward( n, simulate_path_col[i],
                                                 black_reward );
                } else {
                        mcts_node_backup_reward( n, simulate_path_col[i],
                                                 white_reward );
                }
        }
}

/* Run iterations number of simulations for the MCTS tree at root. Backup all
 * reward information.
 *
 * Each simulation ends with one of the conditions
 * - Winner is found. Then the reward is the game result.
 * - A new node needs to expand in the tree. Then the reward is the
 *   predicated_reward of the new node (predicted by the NN).
 *
 * The larger iterations number is the deeper MCTS tree can see the future.
 * Then the result is better.
 */
void
mcts_run_simulation( MCTSNode *root, int iterations )
{
        /* Record path of the simulation for backing up rewards. */
        int       simulate_len;
        MCTSNode *simulate_path_node[MAX_MCTS_SIMULATE_PATH_LEN];
        int       simulate_path_col[MAX_MCTS_SIMULATE_PATH_LEN];

        /* Progress report */
        time_t last_report_progress = time( NULL );

        for ( int it = 0; it < iterations; it++ ) {
                MCTSNode *node = root;

                simulate_len = 0;

                while ( 1 ) {
                        int col = mcts_node_select_next_col_to_evaluate( node );
                        int row = game_legal_row( node->game_snapshot, col );
                        assert( row != -1 );
                        simulate_path_node[simulate_len] = node;
                        simulate_path_col[simulate_len]  = col;
                        simulate_len++;

                        Color next_player =
                            node->game_snapshot->next_player == BLACK ? WHITE
                                                                      : BLACK;
                        Game *dup_game =
                            game_dup_snapshot( node->game_snapshot );
                        dup_game->board[COL_ROW_TO_IDX( col, row )] =
                            dup_game->next_player;
                        dup_game->next_player = next_player;

                        int winner = game_winner( dup_game );

                        /* Found winner */
                        if ( winner >= 0 ) {
                                f32 black_reward;
                                f32 white_reward;
                                if ( winner == 0 ) {
                                        black_reward = 0.f;
                                } else if ( winner == BLACK ) {
                                        black_reward = 1.f;
                                } else {
                                        black_reward = -1.f;
                                }
                                white_reward = -1 * black_reward;
                                mcts_backup_rewards(
                                    black_reward, white_reward, simulate_len,
                                    simulate_path_node, simulate_path_col );
                                game_free( dup_game );
                                break; /* End of this iteration. */
                        }

                        /* Expand new leaf */
                        if ( node->c[col] == NULL ) {
                                MCTSNode *expanded_node = mcts_node_new(
                                    /*moved_in*/ dup_game, node->nn );
                                node->c[col] =
                                    expanded_node; /* owned by node */
                                f32 black_reward;
                                f32 white_reward;
                                black_reward = expanded_node->predicated_reward;
                                if ( expanded_node->game_snapshot
                                         ->next_player == WHITE )
                                        black_reward *= -1.f;
                                white_reward = -1 * black_reward;
                                mcts_backup_rewards(
                                    black_reward, white_reward, simulate_len,
                                    simulate_path_node, simulate_path_col );
                                break; /* End of this iteration. */
                        }

                        game_free( dup_game );
                        /* Keeps playing in this iteration. */
                        node = node->c[col];
                }

                /* Report the progress.
                 *
                 * To avoid over-spamming, the condition is
                 * - First iteration
                 * - Last iteration
                 * - at least 2 seconds have passed.
                 */
                time_t now = time( NULL );
                if ( it == 0 || now - last_report_progress >= 2 ||
                     it == iterations - 1 ) {
                        last_report_progress = now;
                        float progress =
                            (f32)( it + 1 ) / (f32)iterations * 100.f;
                        printf( "MCTS Simulation Progress [#%d]: [%5.1f%%]\n",
                                (int)iterations, progress );
                }
        }
}

/* === Game related utilities ----------------------------------------------- */

/* Creates a new game and initializes nn player randomly. */
Game *
game_new( void )
{
        Game *g = (Game *)calloc( 1, sizeof( *g ) );
        assert( g != NULL );
        g->next_player = BLACK;
        g->nn_player   = ( ( rand( ) % 2 == 0 ) ? BLACK : WHITE );
        return g;
}

void
game_free( Game *g )
{
        if ( g == NULL ) return;
        free( g );
}

Game *
game_dup_snapshot( Game *g )
{
        Game *ng = (Game *)malloc( sizeof( *ng ) );
        assert( ng != NULL );
        memcpy( ng, g, sizeof( *ng ) );
        return ng;
}

/* Display the board. */
void
show_board( Game *g )
{
        printf( "  " );
        for ( int i = 0; i < COLS; i++ ) printf( " %d", i + 1 );
        printf( "\n" );

        for ( int y = 0; y < ROWS; y++ ) {
                printf( "%d ", y + 1 );
                for ( int x = 0; x < COLS; x++ ) {
                        Color c = g->board[COL_ROW_TO_IDX( x, y )];
                        if ( c == NA )
                                printf( " ." );
                        else if ( c == BLACK )
                                printf( " x" );
                        else if ( c == WHITE )
                                printf( " o" );
                }
                printf( "\n" );
        }
}

/* Return the next legal row to place a stone in column (col) or -1 if no way.
 */
int
game_legal_row( Game *g, int col )
{
        Color *board = g->board;
        for ( int row = ROWS - 1; row >= 0; row-- ) {
                if ( board[COL_ROW_TO_IDX( col, row )] == NA ) return row;
        }
        return -1;
}

/* Return BLACK or WHITE if winner exists, 0 if tie, -1 if game is still
 * ongoing.
 */
int
game_winner( Game *g )
{
        Color *board = g->board;
        assert( (int)BLACK > 0 );
        assert( (int)WHITE > 0 );
        /* Pass 1: check winner. */
        for ( int row = 0; row < ROWS; row++ ) {
                for ( int col = 0; col < COLS; col++ ) {
                        Color c = board[COL_ROW_TO_IDX( col, row )];
                        if ( c == NA ) continue;

                        /* Go right */
                        {
                                int i = 1;
                                for ( ; i <= 3; i++ ) {
                                        if ( col + i >= COLS ) break;
                                        int idx =
                                            COL_ROW_TO_IDX( col + i, row );
                                        if ( board[idx] != c ) break;
                                }

                                if ( i == 4 ) return (int)c;
                        }

                        /* Go down */
                        {
                                int i = 1;
                                for ( ; i <= 3; i++ ) {
                                        if ( row + i >= ROWS ) break;
                                        int idx =
                                            COL_ROW_TO_IDX( col, row + i );
                                        if ( board[idx] != c ) break;
                                }

                                if ( i == 4 ) return (int)c;
                        }

                        /* Go up right */
                        {
                                int i = 1;
                                for ( ; i <= 3; i++ ) {
                                        if ( row - i < 0 ) break;
                                        if ( col + i >= COLS ) break;
                                        int idx =
                                            COL_ROW_TO_IDX( col + i, row - i );
                                        if ( board[idx] != c ) break;
                                }

                                if ( i == 4 ) return (int)c;
                        }

                        /* Go down right */
                        {
                                int i = 1;
                                for ( ; i <= 3; i++ ) {
                                        if ( col + i >= COLS ) break;
                                        if ( row + i >= ROWS ) break;
                                        int idx =
                                            COL_ROW_TO_IDX( col + i, row + i );
                                        if ( board[idx] != c ) break;
                                }

                                if ( i == 4 ) return (int)c;
                        }
                }
        }

        /* Pass 2: check tie. */
        int tie = 1;
        for ( int x = 0; x < ROWS * COLS; x++ ) {
                if ( board[x] == NA ) {
                        tie = 0;
                        break;
                }
        }

        if ( tie ) return 0;

        /* Still ongoing */
        return -1;
}

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
        convert_game_to_tensor_input( &in, g );

        /* Call nn
         *
         * The output specification:
         *
         * - The output is a single tensor with ROWS*COLS value. Each is a
         *   probability of the position to place next stone (even the position
         *   is not legal move)
         * - The index of each probability is same as COL_ROW_TO_IDX. Top left
         *   corner is (col=0,row=0).
         */
        Tensor *out; /* Policy output. */
        Tensor *value_out;
        nn_forward( nn, in, &out, &value_out );
        RESET_TENSOR( value_out );
        RESET_TENSOR( in );

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

        RESET_TENSOR( out );
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

void
play_game( NN *nn )
{
        Game *g = game_new( );

        show_board( g );
        while ( 1 ) {
                int col, row;

                if ( g->next_player == g->nn_player ) {
                        // col = policy_random_move( g );
                        // col = policy_nn_move( g, nn );
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

/* === Main ----------------------------------------------------------------- */

int
main( void )
{
        srand( (unsigned)time( NULL ) );
        NN *nn = nn_new( BIN_DATA_FILE );
        play_game( nn );
        nn_free( nn );
}
