// deep_wonders:v1
#include "mcts.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

namespace deep_wonders {

MCTSNode *
mcts_node_new( /*moved_in*/ Game *game_snapshot, NN *nn )
{
        MCTSNode *n = (MCTSNode *)calloc( 1, sizeof( *n ) );
        assert( n != NULL );
        n->game_snapshot = game_snapshot;
        n->nn            = nn;
        n->total_count   = 0;

        /* Initialize priors via NN. */
        float value;
        nn_evaluate( nn, game_snapshot, n->p, &value );

        for ( int i = 0; i < NUM_ACTIONS; i++ ) {
                n->children[i]    = NULL;
                n->visit_count[i] = 0;
                n->w[i]           = 0.0f;
        }
        return n;
}

void
mcts_node_free( MCTSNode *n )
{
        if ( n == NULL ) return;
        for ( int i = 0; i < NUM_ACTIONS; i++ ) {
                mcts_node_free( n->children[i] );
        }
        delete n->game_snapshot;
        free( n );
}

int
mcts_search( Game *g, NN * /*nn*/, int /*iterations*/ )
{
        /* Stub: return a random legal action. */
        int actions[NUM_ACTIONS];
        int n = g->LegalActions( actions );
        return actions[rand() % n];
}

}  // namespace deep_wonders
