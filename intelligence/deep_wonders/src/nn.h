// vim: ft=cpp
// deep_wonders:v1
#pragma once

#include "game.h"

namespace deep_wonders {

typedef struct {
        int placeholder; /* Stub: no real weights yet. */
} NN;

/// Create a new neural network (stub).
NN *nn_new( void );

/// Free the neural network.
void nn_free( NN *nn );

/// Evaluate the game state. Outputs uniform policy and value=0.
///
/// - policy_out: array of size NUM_ACTIONS, filled with uniform probabilities.
/// - value_out: pointer to a single float, set to 0.
void nn_evaluate( NN *nn, Game *g, float *policy_out, float *value_out );

}  // namespace deep_wonders
