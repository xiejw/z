#include "policy.h"

#include <cassert>
#include <vector>

#include "model.h"

namespace c4 {
namespace {
std::vector<int>
legal_moves( const std::span<int> &board )
{
        std::vector<int> pos( COL_COUNT );
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
}  // namespace

int
policy_call( const std::span<int> &b, int next_player_color )
{
        f32_t  pos_to_play_probs[BOARD_SIZE];
        f32_t  winning_prob;
        f32_t *ptr_pos_to_play_probs = pos_to_play_probs;

        assert( b.size( ) == BOARD_SIZE );

        // Call model
        error_t err = c4::model_predict(
            /*next_player_color=*/next_player_color,
            /*board=*/b.data( ),
            /*pos_count=*/BOARD_SIZE,
            /* outputs */
            /*prob=*/&ptr_pos_to_play_probs,
            /*value=*/&winning_prob );
        assert( err == OK );

        DEBUG( ) << "winning probabilty from model: " << winning_prob << "\n";

        auto legal_moves_pos = legal_moves( b );

        int   pos_max = -1;
        f32_t v_max   = 0;
        for ( auto i : legal_moves_pos ) {
                f32_t v = pos_to_play_probs[i];
                if ( v >= v_max ) {
                        pos_max = i;
                        v_max   = v;
                }
        }

        DEBUG( ) << "best pos for next move is " << pos_max
                 << " with prob to be selected " << v_max << "\n";

        return pos_max;
}
}  // namespace c4
