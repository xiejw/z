// vim: ft=cpp
//
// === Learner ------------------------------------------------------------- ===

namespace eos::gan {

struct RLLearner {
      public:
        constexpr static f32 LEARNING_RATE = 0.1f;

      private:
#define cyan  "\x1b[36m"
#define yelw  "\x1b[35m"
#define reset "\x1b[0m"

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

                INFO( "Training neural network against {} random games...",
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
                        if ( ( i < 10000 && ( i + 1 ) % 1000 == 0 ) ||
                             ( i + 1 ) % 10000 == 0 ) {
                                INFO(
                                    "Training progress: {} ({:.1f}%) "
                                    "(NN starts {:.1f}%), "
                                    "Wins: {} (" cyan "{:.1f}%)" reset
                                    ", "
                                    "Losses: {} ({:.1f}%), "
                                    "Ties: {} (" yelw "{:.1f}%)" reset,
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
                INFO( "Training complete!" );
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
                                // move = Sampler::get_best_move( &state, nn );
                                move = Sampler::get_random_move( &state );
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
                        GameState state{ };
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
