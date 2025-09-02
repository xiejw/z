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
      public:
        using Symbol = char;

      private:
        Symbol board[kGameStateCount];  // Can be "." (empty) or "X", "O".
        Symbol symbol_for_next_move = SymBK;

      public:
        constexpr static Symbol SymNA = '.';
        constexpr static Symbol SymBK = 'X';
        constexpr static Symbol SymWT = 'O';

      public:
        GameState( );

      public:
        // Return the board symbol at position i.
        auto get_symbol_at( int i ) const -> Symbol { return board[i]; }

        auto get_symbol_for_next_move( ) const -> Symbol
        {
                return symbol_for_next_move;
        }

        auto place_next_move_at( int i )
        {
                assert( board[i] == SymNA );
                board[i] = symbol_for_next_move;
                symbol_for_next_move =
                    symbol_for_next_move == SymBK ? SymWT : SymBK;
        }

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

GameState::GameState( ) { memset( this->board, SymNA, kGameStateCount ); }

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
        // Hard code some rules
        assert( kGameRowCount == 3 && kGameColCount == 3 );

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
        void backward( f32 *target_probs, f32 lr, f32 reward_scaling );
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

/* Derivative of ReLU activation function */
f32
relu_derivative( f32 x )
{
        return x > 0 ? 1.0f : 0.0f;
}

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
        //
        // Shapes
        // - matmul (inputs / 1 x IN, weights_ih / IN x HIDDEN)
        // - biases_h / HIDDEN
        // - hidden / HIDDEN
        //
        for ( std::size_t i = 0; i < HIDDEN; i++ ) {
                float sum = this->biases_h[i];
                for ( std::size_t j = 0; j < IN; j++ ) {
                        sum += inputs[j] * this->weights_ih[j * HIDDEN + i];
                }
                this->hidden[i] = relu( sum );
        }

        // Hidden to output (raw logits).
        //
        // Shapes
        // - matmul (hidden / 1 x HIDDEN, weights_ho / HIDDEN x OUT )
        // - biases_o / OUT
        // - raw_logits / OUT
        //
        for ( std::size_t i = 0; i < OUT; i++ ) {
                this->raw_logits[i] = this->biases_o[i];
                for ( std::size_t j = 0; j < HIDDEN; j++ ) {
                        this->raw_logits[i] +=
                            this->hidden[j] * this->weights_ho[j * OUT + i];
                }
        }

        // Apply softmax to get the final probabilities.
        //
        // Shapes
        // - raw_logits / OUT
        // - outputs / OUT
        //
        softmax( /*input=*/this->raw_logits, /*output=*/this->outputs, OUT );
}

/* Backpropagation function.
 *
 * The only difference here from vanilla backprop is that we have
 * a 'reward_scaling' argument that makes the output error more/less
 * dramatic, so that we can adjust the weights proportionally to the
 * reward we want to provide.
 */
template <std::size_t IN, std::size_t OUT, std::size_t HIDDEN>
void
NeuralNetwork<IN, OUT, HIDDEN>::backward( f32 *target_probs, f32 lr,
                                          f32 reward_scaling )
{
        f32 raw_logits_deltas[OUT];
        f32 hidden_deltas[HIDDEN];

        /* === --- STEP 1: Compute Deltas ------------------------------- === */

        /* Calculate output layer deltas:
         *
         * Note what's going on here: we are technically using softmax
         * from raw_logits to output in forward function which uses cross
         * entropy as loss.
         *
         * Still calculating the deltas as:
         *
         *      output[i] - target[i]
         *
         * NOTE: This is gradients w.r.t. to raw_logits not outputs (even
         * though it uses outputs only). Check the gradients for cross entropy
         * for details.
         *
         * Shapes
         * - outputs / OUT
         * - target_probs / OUT
         * - raw_logits_deltas / OUT
         *
         */
        for ( std::size_t i = 0; i < OUT; i++ ) {
                raw_logits_deltas[i] = ( this->outputs[i] - target_probs[i] ) *
                                       fabsf( reward_scaling );
        }

        // Backpropagate error to hidden layer.
        //
        // Shapes
        // - matmul(
        //     raw_logits_deltas /  OUT,
        //     trans( weights_ho / HIDDEN x OUT )
        //   )
        // - hidden / 1 x HIDDEN
        // - hidden_deltas / 1 x HIDDEN
        //
        for ( std::size_t i = 0; i < HIDDEN; i++ ) {
                f32 error = 0;
                for ( std::size_t j = 0; j < OUT; j++ ) {
                        error += raw_logits_deltas[j] *
                                 this->weights_ho[i * OUT + j];
                }
                hidden_deltas[i] = error * relu_derivative( this->hidden[i] );
        }

        /* === --- STEP 2: Weights Updating -----------------------------=== */

        // Output layer weights and biases.
        //
        // Shapes
        // - hidden / 1 x HIDDEN
        // - weights_ho / HIDDEN x OUT
        // - raw_logits_deltas / 1 x OUT
        //
        // Forward
        // - hidden ** weights_ho = raw_logits
        // Backward
        // - d_weights_ho = matmul(trans(hidden), d_raw_logits)
        //
        for ( std::size_t i = 0; i < HIDDEN; i++ ) {
                for ( std::size_t j = 0; j < OUT; j++ ) {
                        this->weights_ho[i * OUT + j] -=
                            lr * raw_logits_deltas[j] * this->hidden[i];
                }
        }
        // Shapes
        // - biases_o / 1 x OUT
        // - raw_logits_deltas / 1 x OUT
        //
        for ( std::size_t j = 0; j < OUT; j++ ) {
                this->biases_o[j] -= lr * raw_logits_deltas[j];
        }

        // Hidden layer weights and biases.
        //
        // Shapes
        // - inputs 1 x IN
        // - weights_ih IN x HIDDEN
        // - hidden_deltas 1 x HIDDEN
        //
        // Forward
        // - inputs ** weights_ih = hidden
        // Backward
        // - d_weights_ih = trans(d_inputs) ** hidden
        //
        for ( std::size_t i = 0; i < IN; i++ ) {
                for ( std::size_t j = 0; j < HIDDEN; j++ ) {
                        this->weights_ih[i * HIDDEN + j] -=
                            lr * hidden_deltas[j] * this->inputs[i];
                }
        }
        // Shapes
        // - biases_h / 1 x HIDDEN
        // - hidden_deltas / 1 x HIDDEN
        //
        for ( std::size_t j = 0; j < HIDDEN; j++ ) {
                this->biases_h[j] -= lr * hidden_deltas[j];
        }
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
                        if ( state->get_symbol_at( i ) == GameState::SymNA &&
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

// === Learner ------------------------------------------------------------- ===

namespace eos::gan {

struct RLLearner {
      public:
        constexpr static f32 LEARNING_RATE = 0.1f;

      public:
        template <std::size_t IN, std::size_t OUT, std::size_t HIDDEN>
        static void train_against_random( NeuralNetwork<IN, OUT, HIDDEN> *nn,
                                          int num_games )
        {
                int move_history[kGameStateCount];
                int wins            = 0;
                int losses          = 0;
                int ties            = 0;
                int nn_starts_first = 0;

                std::print(
                    "Training neural network against {} random games...\n",
                    num_games );

                int played_games = 0;
                for ( int i = 0; i < num_games; i++ ) {
                        GameState::Symbol nn_symbol;
                        GameState::Symbol winner =
                            play_random_game( nn, move_history, &nn_symbol );

                        played_games++;
                        nn_starts_first += ( nn_symbol == GameState::SymBK );

                        // Accumulate statistics that are provided to the user
                        // (it's fun).
                        if ( winner == nn_symbol ) {
                                wins++;  // Neural network won.
                        } else if ( winner == GameState::SymNA ) {
                                ties++;  // Tie.
                        } else {
                                losses++;  // Random player won.
                        }

                        // Show progress every many games to avoid flooding the
                        // stdout.
                        if ( ( i + 1 ) % 10000 == 0 ) {
                                std::print(
                                    "Games progress: {} ({:.1f}%) "
                                    "(NN starts {:.1f}%), "
                                    "Wins: {} ({:.1f}%), "
                                    "Losses: {} ({:.1f}%), "
                                    "Ties: {} ({:.1f}%)"
                                    "\n",
                                    i + 1,
                                    f32( i + 1 ) * 100.f / f32( num_games ),
                                    (float)nn_starts_first * 100.0f /
                                        (float)( i + 1 ),
                                    wins,
                                    (float)wins * 100.f / (float)played_games,
                                    losses,
                                    (float)losses * 100.f / (float)played_games,
                                    ties,
                                    (float)ties * 100.f / (float)played_games );
                                played_games = 0;
                                wins         = 0;
                                losses       = 0;
                                ties         = 0;
                        }
                }
                std::print( "\nTraining complete!\n" );
        }

      private:
        /*
         * This is a very simple Montecarlo Method applied to reinforcement
         * learning:
         *
         * 1. We play a complete random game (episode).
         * 2. We determine the reward based on the outcome of the game.
         * 3. We update the neural network in order to maximize future
         rewards.
         */
        template <std::size_t IN, std::size_t OUT, std::size_t HIDDEN>
        static GameState::Symbol play_random_game(
            NeuralNetwork<IN, OUT, HIDDEN> *nn, int *move_history,
            GameState::Symbol *pout_nn )
        {
                GameState         state{ };
                GameState::Symbol winner;
                int               num_moves = 0;

                GameState::Symbol nn_symbol =
                    ( rand( ) & 1 ) == 0 ? GameState::SymBK : GameState::SymWT;
                *pout_nn = nn_symbol;

                while ( true ) {
                        auto winner_opt = state.is_game_over( );
                        if ( bool( winner_opt ) ) {
                                winner = winner_opt.value( );
                                break;
                        }
                        int move;

                        if ( state.get_symbol_for_next_move( ) !=
                             nn_symbol ) {  // Random player's turn

                                // TODO
                                // move = PLAY_RANDOM_GAME_AGAINST_NN
                                //            ? Sampler::get_best_move( &state,
                                //            nn) : get_random_move( &state );
                                move = Sampler::get_best_move( &state, nn );
                        } else {  // Neural network's turn
                                move = Sampler::get_best_move( &state, nn );
                        }

                        /* Make the move and store it: we need the moves
                         * sequence during the learning stage. */
                        state.place_next_move_at( move );
                        move_history[( num_moves )++] = move;
                }

                // Learn from this game
                update_nn( nn, move_history, num_moves, nn_symbol, winner );
                return winner;
        }

        /* Update the neural network based on game outcome.
         *
         * The move_history is just an integer array with the index of all
         * the moves. This function is designed so that you can specify if the
         * game was started by the move by the NN or human, but actually the
         * code always let the human move first. */
        template <std::size_t IN, std::size_t OUT, std::size_t HIDDEN>
        static void update_nn( NeuralNetwork<IN, OUT, HIDDEN> *nn,
                               int *move_history, int num_moves,
                               GameState::Symbol nn_symbol,
                               GameState::Symbol winner )
        {
                // Determine reward based on game outcome
                float reward;

                int nn_moves_even = nn_symbol == GameState::SymWT;

                if ( winner == GameState::SymNA ) {
                        reward = 0.3f;  // Small reward for draw
                } else if ( winner == nn_symbol ) {
                        reward = 1.0f;  // Large reward for win
                } else {
                        reward = -2.0f;  // Negative reward for loss
                }

                f32 target_probs[OUT];

                // Process each move the neural network made.
                for ( int move_idx = 0; move_idx < num_moves; move_idx++ ) {
                        // Skip if this wasn't a move by the neural network.
                        if ( ( nn_moves_even && move_idx % 2 != 1 ) ||
                             ( !nn_moves_even && move_idx % 2 != 0 ) ) {
                                continue;
                        }

                        // Recreate board state BEFORE this move was made.
                        GameState state;
                        for ( int i = 0; i < move_idx; i++ ) {
                                state.place_next_move_at( move_history[i] );
                        }

                        // Convert board to inputs and do forward pass.
                        float inputs[IN];
                        state.convert_board_to_inputs( inputs );
                        nn->forward( inputs );

                        /* The move that was actually made by the NN, that is
                         * the one we want to reward (positively or negatively).
                         */
                        int move = move_history[move_idx];

                        /* Here we can't really implement temporal difference in
                         * the strict reinforcement learning sense, since we
                         * don't have an easy way to evaluate if the current
                         * situation is better or worse than the previous state
                         * in the game.
                         *
                         * However "time related" we do something that is very
                         * effective in this case: we scale the reward according
                         * to the move time, so that later moves are more
                         * impacted (the game is less open to different
                         * solutions as we go forward).
                         *
                         * We give a fixed 0.5 importance to all the moves plus
                         * a 0.5 that depends on the move position.
                         *
                         * NOTE: this makes A LOT of difference. Experiment with
                         * different values.
                         */
                        f32 move_importance =
                            0.5f + 0.5f * (f32)move_idx / (f32)num_moves;
                        f32 scaled_reward = reward * move_importance;

                        /* Create target probability distribution:
                         * let's start with the logits all set to 0. */
                        for ( std::size_t i = 0; i < OUT; i++ )
                                target_probs[i] = 0;

                        /* Set the target for the chosen move based on reward:
                         */
                        if ( scaled_reward >= 0 ) {
                                /* For positive reward, set probability of the
                                 * chosen move to 1, with all the rest set to 0.
                                 */
                                target_probs[move] = 1.0f;
                        } else {
                                /* For negative reward, distribute probability
                                 * to OTHER valid moves, which is conceptually
                                 * the same as discouraging the move that we
                                 * want to discourage. */
                                int   valid_moves_left = 9 - move_idx - 1;
                                float other_prob =
                                    1.0f / (float)valid_moves_left;
                                for ( int i = 0; i < 9; i++ ) {
                                        if ( state.get_symbol_at( i ) ==
                                                 GameState::SymNA &&
                                             i != move ) {
                                                target_probs[i] = other_prob;
                                        }
                                }
                        }

                        /* Call the generic backpropagation function, using
                         * our target logits as target. */
                        nn->backward( target_probs, LEARNING_RATE,
                                      scaled_reward );
                }
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
        nn.backward( inputs, /*lr=*/0.1f, /*reward_scaling=*/1.0f );

        eos::gan::Sampler::get_best_move( &s, &nn );
        eos::gan::RLLearner::train_against_random( &nn, 150000 );
        return 0;
}
