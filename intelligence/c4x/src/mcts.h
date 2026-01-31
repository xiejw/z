// vim: ft=cpp
// forge:v1
// hermes:v1
#pragma once

#include "game.h"
#include "nn.h"
#include "tensor.h"

namespace hermes {

/* === MCTS node and tree --------------------------------------------------- */

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

/// During creating, NN is invoked to provide predicated_reward (chance to win)
/// and prior probabilities for all legal moves.
MCTSNode *mcts_node_new( /*moved_in*/ Game *game_snapshot, NN *nn );

/// Recursively free the entire MCTS tree rooted at n.
void mcts_node_free( MCTSNode *n );

/// Run iterations number of simulations for the MCTS tree at root. Backup all
/// reward information.
///
/// Each simulation ends with one of the conditions
/// - Winner is found. Then the reward is the game result.
/// - A new node needs to expand in the tree. Then the reward is the
///   predicated_reward of the new node (predicted by the NN).
///
/// The larger iterations number is the deeper MCTS tree can see the future.
/// Then the result is better.
void mcts_run_simulation( MCTSNode *root, int iterations );

// Select the next column to play. Currently, choose the one with most visited
// count.
///
int mcts_node_select_next_col_to_play( MCTSNode *node );
}  // namespace hermes
