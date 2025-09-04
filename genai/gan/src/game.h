// vim: ft=cpp
//
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
