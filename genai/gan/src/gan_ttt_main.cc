#include <zion/zion.h>

namespace eos::gan {
// Neural Network Paramters.
constexpr int kNNInputSize  = 18;
constexpr int kNNHiddenSize = 100;
constexpr int kNNOutputSize = 9;
// constexpr f32 kLearningRate = 0.1f;

// Game board representation.
class GameState {
      public:
        char board[9];
};

// Two-layer Neural network.
class NeuralNetwork {
      public:
        // Weights and biases.
        float weights_ih[kNNInputSize * kNNHiddenSize];
        float weights_ho[kNNHiddenSize * kNNOutputSize];
        float biases_h[kNNHiddenSize];
        float biases_o[kNNOutputSize];

        // Activations are part of the structure itself for simplicity.
        float inputs[kNNInputSize];
        float hidden[kNNHiddenSize];
        float raw_logits[kNNOutputSize];  // Outputs before softmax().
        float outputs[kNNOutputSize];     // Outputs after softmax().
                                          //
      public:
        void init( );
};

namespace {
/* Initialize a neural network with random weights, we should
 * use something like He weights since we use RELU, but we don't
 * care as this is a trivial example. */
inline f32
rand_f32( )
{
        return ( ( (float)rand( ) / (float)RAND_MAX ) - 0.5f );
}

}  // namespace

void
NeuralNetwork::init( )
{
        // Initialize weights with random values between -0.5 and 0.5
        for ( int i = 0; i < kNNInputSize * kNNHiddenSize; i++ )
                this->weights_ih[i] = rand_f32( );

        for ( int i = 0; i < kNNHiddenSize * kNNOutputSize; i++ )
                this->weights_ho[i] = rand_f32( );

        for ( int i = 0; i < kNNHiddenSize; i++ )
                this->biases_h[i] = rand_f32( );

        for ( int i = 0; i < kNNOutputSize; i++ )
                this->biases_o[i] = rand_f32( );
}

}  // namespace eos::gan

int
main( )
{
        eos::gan::NeuralNetwork nn{ };
        nn.init( );
        return 0;
}
