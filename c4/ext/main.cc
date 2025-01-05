#include "model.h"

#include <cassert>
#include <vector>

//
// internal helper
//
static std::vector<int> legal_moves( const std::vector<int> &board );

//
// apis
//
int
policy_lookahead_onestep( const std::vector<int> &b, int next_player_color )
{
        f32_t  probs[BOARD_SIZE];
        f32_t  value;
        f32_t *ptr_probs = probs;

        // call model
        error_t err = c4::model_predict(
            /*next_player_color=*/next_player_color,
            /*board=*/b.data( ),
            /*pos_count=*/BOARD_SIZE,
            /* outputs */
            /*prob=*/&ptr_probs,
            /*value=*/&value );
        assert( err == OK );

        DEBUG( ) << "winning probabilty from model: " << value << "\n";

        auto legal_moves_pos = legal_moves( b );

        int   pos_max = -1;
        f32_t v_max   = 0;
        for ( auto i : legal_moves_pos ) {
                f32_t v = probs[i];
                if ( v >= v_max ) {
                        pos_max = i;
                        v_max   = v;
                }
        }

        DEBUG( ) << "best pos for next move is " << pos_max
                 << " with prob to be selected " << v_max << "\n ";

        return pos_max;
}

int
main( )
{
        return 0;
}

//
// internal helper impl
//
std::vector<int>
legal_moves( const std::vector<int> &board )
{
        std::vector<int> pos{ };
        for ( int col = 0; col < COL_COUNT; col++ ) {
                for ( int row = ROW_COUNT - 1; row >= 0; row-- ) {
                        int index = row * COL_COUNT + col;
                        if ( board[size_t( index )] == NA_INT ) {
                                pos.push_back( index );
                                break;
                        }
                }
        }
        assert( pos.size( ) > 0 );
        return pos;
}
