// vim: ft=cpp
//
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

      public:
        /* Get a random valid move, this is used for training against a random
         * opponent. Note: this function will loop forever if the board is
         * full, but here we want simple code.
         */
        static auto get_random_move( GameState *state ) -> int
        {
                while ( 1 ) {
                        int move = rand( ) % kGameStateCount;
                        if ( state->get_symbol_at( move ) != GameState::SymNA )
                                continue;
                        return move;
                }
        }
};
}  // namespace eos::gan
