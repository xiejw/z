// https://github.com/xiejw/z/commit/fd326f22906da2855497008aaedf0f3aa9590f8f
#include <zion/zion.h>

#include <cmath>
#include <cstring>
#include <print>
#include <span>

// === Game Board State ---------------------------------------------------- ===

namespace eos::gan {
constexpr int kGameRowCount   = 3;
constexpr int kGameColCount   = 3;
constexpr int kGameStateCount = kGameRowCount * kGameColCount;

// Game board representation.
class GameState {
      private:
        char board[kGameStateCount];  // Can be "." (empty) or "X", "O".

      public:
        constexpr static char CharNotSet = '.';
        constexpr static char CharBlack  = 'X';
        constexpr static char CharWhite  = 'O';

      public:
        GameState( );

      public:
        // Show board on screen in ASCII "art".
        auto display_board( ) -> void;

        // Convert board state to neural network inputs.
        //
        // Instead of one-hot encoding, we can represent N different categories
        // as different bit patterns. In this specific case it's trivial:
        //
        // 00 = empty
        // 10 = X
        // 01 = O
        //
        // Two inputs per symbol instead of 3 in this case, but in the general
        // case this reduces the input dimensionality A LOT.
        auto convert_board_to_inputs( std::span<f32> &inputs ) const -> void;
};

GameState::GameState( ) { memset( this->board, '.', kGameStateCount ); }

auto
GameState::display_board( ) -> void
{
        for ( int row = 0; row < kGameRowCount; row++ ) {
                // Display the board symbols.
                std::print( "{}{}{} ", this->board[row * kGameColCount],
                            this->board[row * kGameColCount + 1],
                            this->board[row * kGameColCount + 2] );

                // Display the position numbers for this row, for the poor
                // human.
                std::print( "{}{}{}\n", row * kGameColCount,
                            row * kGameColCount + 1, row * kGameColCount + 2 );
        }
        std::print( "\n" );
}
auto
GameState::convert_board_to_inputs( std::span<f32> &inputs ) const -> void
{
        if ( inputs.size( ) < kGameStateCount * 2 ) {
                PANIC( "inputs are not big enough: expect: {} got: {}",
                       kGameStateCount * 2, inputs.size( ) );
        }

        f32 *data = inputs.data( );
        for ( int i = 0; i < 9; i++ ) {
                if ( this->board[i] == CharNotSet ) {
                        data[i * 2]     = 0;
                        data[i * 2 + 1] = 0;
                } else if ( this->board[i] == CharBlack ) {
                        data[i * 2]     = 1;
                        data[i * 2 + 1] = 0;
                } else {
                        assert( this->board[i] == CharWhite );
                        data[i * 2]     = 0;
                        data[i * 2 + 1] = 1;
                }
        }
}
}  // namespace eos::gan

// === NeuralNetwork ------------------------------------------------------- ===

namespace eos::gan {
// Neural Network Paramters.
constexpr int kNNInputSize  = kGameStateCount * 2;
constexpr int kNNHiddenSize = 100;
constexpr int kNNOutputSize = kGameStateCount;
// constexpr f32 kLearningRate = 0.1f;

// Two-layer Neural network.
template <std::size_t IN = kNNInputSize, std::size_t OUT = kNNOutputSize,
          std::size_t HIDDEN = kNNHiddenSize>
class NeuralNetwork {
      public:
        // Weights and biases.
        float weights_ih[IN * HIDDEN];
        float weights_ho[HIDDEN * OUT];
        float biases_h[HIDDEN];
        float biases_o[OUT];

        // Activations are part of the structure itself for simplicity.
        float inputs[IN];
        float hidden[HIDDEN];
        float raw_logits[OUT];  // Outputs before softmax().
        float outputs[OUT];     // Outputs after softmax().
                                //
      public:
        void init( );
        void forward( f32 * );
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

/* ReLU activation function */
float
relu( float x )
{
        return x > 0 ? x : 0;
}

///* Derivative of ReLU activation function */
// float
// relu_derivative( float x )
//{
//         return x > 0 ? 1.0f : 0.0f;
// }

/* Apply softmax activation function to an array input, and
 * set the result into output. */
void
softmax( float *input, float *output, int size )
{
        /* Find maximum value then subtact it to avoid
         * numerical stability issues with exp(). */
        float max_val = input[0];
        for ( int i = 1; i < size; i++ ) {
                if ( input[i] > max_val ) {
                        max_val = input[i];
                }
        }

        // Calculate exp(x_i - max) for each element and sum.
        float sum = 0.0f;
        for ( int i = 0; i < size; i++ ) {
                output[i] = expf( input[i] - max_val );
                sum += output[i];
        }

        // Normalize to get probabilities.
        if ( sum > 0 ) {
                for ( int i = 0; i < size; i++ ) {
                        output[i] /= sum;
                }
        } else {
                /* Fallback in case of numerical issues, just provide
                 * a uniform distribution. */
                for ( int i = 0; i < size; i++ ) {
                        output[i] = 1.0f / (float)size;
                }
        }
}

}  // namespace

template <std::size_t IN, std::size_t OUT, std::size_t HIDDEN>
void
NeuralNetwork<IN, OUT, HIDDEN>::init( )
{
        // Initialize weights with random values between -0.5 and 0.5
        for ( std::size_t i = 0; i < IN * HIDDEN; i++ )
                this->weights_ih[i] = rand_f32( );

        for ( std::size_t i = 0; i < HIDDEN * OUT; i++ )
                this->weights_ho[i] = rand_f32( );

        for ( std::size_t i = 0; i < HIDDEN; i++ )
                this->biases_h[i] = rand_f32( );

        for ( std::size_t i = 0; i < OUT; i++ ) this->biases_o[i] = rand_f32( );
}

/* Neural network foward pass (inference).
 *
 * NOTE: The activations are stored so we can also do backpropagation later.
 */
template <std::size_t IN, std::size_t OUT, std::size_t HIDDEN>
void
NeuralNetwork<IN, OUT, HIDDEN>::forward( f32 *inputs )
{
        // Copy inputs.
        memcpy( this->inputs, inputs, IN * sizeof( float ) );

        // Input to hidden layer.
        for ( std::size_t i = 0; i < HIDDEN; i++ ) {
                float sum = this->biases_h[i];
                for ( std::size_t j = 0; j < IN; j++ ) {
                        sum += inputs[j] * this->weights_ih[j * HIDDEN + i];
                }
                this->hidden[i] = relu( sum );
        }

        // Hidden to output (raw logits).
        for ( std::size_t i = 0; i < OUT; i++ ) {
                this->raw_logits[i] = this->biases_o[i];
                for ( std::size_t j = 0; j < HIDDEN; j++ ) {
                        this->raw_logits[i] +=
                            this->hidden[j] * this->weights_ho[j * OUT + i];
                }
        }

        // Apply softmax to get the final probabilities.
        softmax( this->raw_logits, this->outputs, OUT );
}

}  // namespace eos::gan

int
main( )
{
        eos::gan::GameState s{ };
        s.display_board( );

        eos::gan::NeuralNetwork nn{ };
        nn.init( );

        f32 inputs[eos::gan::kNNInputSize];
        nn.forward( inputs );
        return 0;
}
