// vim: ft=cpp
// deep_wonders:v1
#pragma once

#include "game.h"
#include "nn.h"

namespace deep_wonders {

typedef struct MCTSNode {
        Game *game_snapshot; /* Owned */
        NN   *nn;            /* Unowned */

        int total_count;

        /* Owned children for each legal action. NULL means unexpanded. */
        struct MCTSNode *children[NUM_ACTIONS];

        /* Visit count for each action. */
        int visit_count[NUM_ACTIONS];
        /* Backed up total value for each action. */
        float w[NUM_ACTIONS];
        /* Prior probability for each action. */
        float p[NUM_ACTIONS];
} MCTSNode;

/// Create a new MCTS node. Takes ownership of game_snapshot.
MCTSNode *mcts_node_new( /*moved_in*/ Game *game_snapshot, NN *nn );

/// Recursively free the MCTS tree rooted at n.
void mcts_node_free( MCTSNode *n );

/// Run MCTS search and return the best action.
/// Currently returns a random legal action (stub).
int mcts_search( Game *g, NN *nn, int iterations );

}  // namespace deep_wonders
