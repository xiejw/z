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
        using Symbol                  = char;
        constexpr static Symbol SymNA = '.';
        constexpr static Symbol SymBK = 'X';
        constexpr static Symbol SymWT = 'O';

      public:
        GameState( );

      public:
        // Return the board symbol at position i.
        auto get_board_symbol( int i ) -> Symbol { return board[i]; }

        // Show board on screen in ASCII "art".
        auto display_board( ) -> void;

        // Convert board state to neural network inputs.
        //
        // Instead of one-hot encoding, we can represent N different categories
        // as different bit patterns. In this specific case it's trivial:
        //
        // 00 = SymNA (empty)
        // 10 = SymBK (X)
        // 01 = SymWT (O)
        //
        // Two inputs per symbol instead of 3 in this case, but in the general
        // case this reduces the input dimensionality A LOT.
        auto convert_board_to_inputs( std::span<f32> inputs ) const -> void;

        /* Check if the game is over (win or tie).
         * Return None if game is not over
         * Return SymNA if tie. SymBK or SymWT if winner exists.
         */
        auto is_game_over( ) -> std::optional<Symbol>;
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
GameState::convert_board_to_inputs( std::span<f32> inputs ) const -> void
{
        if ( inputs.size( ) < kGameStateCount * 2 ) {
                PANIC( "inputs are not big enough: expect: {} got: {}",
                       kGameStateCount * 2, inputs.size( ) );
        }

        f32 *data = inputs.data( );
        for ( int i = 0; i < kGameStateCount; i++ ) {
                if ( this->board[i] == SymNA ) {
                        data[i * 2]     = 0;
                        data[i * 2 + 1] = 0;
                } else if ( this->board[i] == SymBK ) {
                        data[i * 2]     = 1;
                        data[i * 2 + 1] = 0;
                } else {
                        assert( this->board[i] == SymWT );
                        data[i * 2]     = 0;
                        data[i * 2 + 1] = 1;
                }
        }
}

auto
GameState::is_game_over( ) -> std::optional<Symbol>
{
        // Check rows.
        for ( int i = 0; i < 3; i++ ) {
                if ( this->board[i * 3] != SymNA &&
                     this->board[i * 3] == this->board[i * 3 + 1] &&
                     this->board[i * 3 + 1] == this->board[i * 3 + 2] ) {
                        return this->board[i * 3];
                }
        }

        // Check columns.
        for ( int i = 0; i < 3; i++ ) {
                if ( this->board[i] != SymNA &&
                     this->board[i] == this->board[i + 3] &&
                     this->board[i + 3] == this->board[i + 6] ) {
                        return this->board[i];
                }
        }

        // Check diagonals.
        if ( this->board[0] != SymNA && this->board[0] == this->board[4] &&
             this->board[4] == this->board[8] ) {
                return this->board[0];
        }
        if ( this->board[2] != SymNA && this->board[2] == this->board[4] &&
             this->board[4] == this->board[6] ) {
                return this->board[2];
        }

        // Check for tie (no free tiles left).
        int empty_tiles = 0;
        for ( int i = 0; i < kGameStateCount; i++ ) {
                if ( this->board[i] == SymNA ) empty_tiles++;
        }
        if ( empty_tiles == 0 ) {
                return SymNA;  // Tie
        }

        return std::nullopt;  // Game continues.
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

// === Sampler ------------------------------------------------------------- ===

namespace eos::gan {

struct Sampler {
      public:
        template <std::size_t IN     = kNNInputSize,
                  std::size_t OUT    = kNNOutputSize,
                  std::size_t HIDDEN = kNNHiddenSize>
        static auto get_best_move( GameState                      *state,
                                   NeuralNetwork<IN, OUT, HIDDEN> *nn ) -> int
        {
                f32 inputs[IN];

                state->convert_board_to_inputs( inputs );
                nn->forward( inputs );

                int   best_move       = -1;
                float best_legal_prob = -1.0f;

                for ( int i = 0; i < kGameStateCount; i++ ) {
                        // Track best legal move.
                        if ( state->get_board_symbol( i ) == GameState::SymNA &&
                             ( best_move == -1 ||
                               nn->outputs[i] > best_legal_prob ) ) {
                                best_move       = i;
                                best_legal_prob = nn->outputs[i];
                        }
                }
                return best_move;
        }
};

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

        eos::gan::Sampler::get_best_move( &s, &nn );
        return 0;
}
