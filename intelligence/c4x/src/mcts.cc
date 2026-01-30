#include "mcts.h"

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <time.h>

#define MCTS_PROB_LOW_LIMIT \
        0.05f /* The low limit we allow for each MCTS node. */

#define RESET_TENSOR( t )         \
        do {                      \
                free_tensor( t ); \
                ( t ) = NULL;     \
        } while ( 0 )

namespace hermes {

namespace {
/// Select the next column to evaluate during simulation.  Read AlphaGoZero
/// paper (2017, "Methods" section, "Select" paragraph) for details.
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
}  // namespace

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

}  // namespace hermes
