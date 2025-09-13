// TODO
// - loop out so reuse
// - apply gradient is out. must have zero. calculate and apply 3 steps.
// - flow grad to input
// https://github.com/xiejw/z/commit/fd326f22906da2855497008aaedf0f3aa9590f8f
#include <zion/zion.h>

// Must import in order. There is no dependency encoded in these header files.
// clang-format off
#include "game.h"
#include "nn.h"
#include "sampler.h"
#include "learner.h"
// clang-format on

int
main( )
{
        srand( (unsigned)time( NULL ) );
        eos::gan::NeuralNetwork nn{ };
        nn.init( );
        eos::gan::RLLearner::train_against_random( &nn, 150000 );
        return 0;
}
