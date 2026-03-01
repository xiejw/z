// deep_wonders:v1
#include "nn.h"

#include <assert.h>
#include <stdlib.h>

namespace deep_wonders {

NN *
nn_new( void )
{
        NN *nn = (NN *)calloc( 1, sizeof( *nn ) );
        assert( nn != NULL );
        return nn;
}

void
nn_free( NN *nn )
{
        if ( nn == NULL ) return;
        free( nn );
}

void
nn_evaluate( NN * /*nn*/, Game *g, float *policy_out, float *value_out )
{
        int n = game_num_actions( g );
        /* Uniform policy. */
        for ( int i = 0; i < n; i++ ) {
                policy_out[i] = 1.0f / (float)n;
        }
        *value_out = 0.0f;
}

}  // namespace deep_wonders
