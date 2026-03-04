// deep_wonders:v1
#include "game.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

namespace deep_wonders {

/* Snake draft order: P0, P1, P1, P0, P1, P0, P0, P1 */
static const int kDraftPlayer[NUM_SELECTED] = { 0, 1, 1, 0, 1, 0, 0, 1 };

/* Card display: 0-9 as '0'-'9', 10 as 'a', 11 as 'b'. */
static char
CardChar( int card )
{
        assert( card >= 0 && card < NUM_WONDERS );
        if ( card < 10 ) return (char)( '0' + card );
        return (char)( 'a' + card - 10 );
}

Game::Game()
        : turn_( 0 ), done_( false ), winner_( -1 )
{
        /* Fisher-Yates shuffle of 0..NUM_WONDERS-1, take first 8. */
        int deck[NUM_WONDERS];
        for ( int i = 0; i < NUM_WONDERS; i++ ) deck[i] = i;
        for ( int i = NUM_WONDERS - 1; i > 0; i-- ) {
                int j  = rand() % ( i + 1 );
                int tmp = deck[i];
                deck[i] = deck[j];
                deck[j] = tmp;
        }
        for ( int i = 0; i < NUM_SELECTED; i++ ) {
                selected_[i] = deck[i];
                picked_[i]   = false;
                owner_[i]    = -1;
        }
}

Game *
Game::Dup() const
{
        Game *ng = new Game( *this );
        return ng;
}

int
Game::NumActions() const
{
        return NUM_ACTIONS;
}

int
Game::LegalActions( int *out ) const
{
        assert( !done_ );
        int count = 0;
        int base  = ( turn_ < BATCH_SIZE ) ? 0 : BATCH_SIZE;
        for ( int i = base; i < base + BATCH_SIZE; i++ ) {
                if ( !picked_[i] ) {
                        out[count++] = ActionEncode( selected_[i], OP_SELECT );
                }
        }
        return count;
}

bool
Game::IsLegalAction( int action ) const
{
        if ( done_ ) return false;
        if ( action < 0 || action >= NUM_ACTIONS ) return false;
        int card = ActionCard( action );
        int op   = ActionOp( action );
        if ( op != OP_SELECT ) return false;

        int base = ( turn_ < BATCH_SIZE ) ? 0 : BATCH_SIZE;
        for ( int i = base; i < base + BATCH_SIZE; i++ ) {
                if ( !picked_[i] && selected_[i] == card ) return true;
        }
        return false;
}

void
Game::ApplyAction( int action )
{
        assert( !done_ );
        assert( IsLegalAction( action ) );

        int card   = ActionCard( action );
        int player = kDraftPlayer[turn_];

        /* Find slot and mark picked. */
        int base = ( turn_ < BATCH_SIZE ) ? 0 : BATCH_SIZE;
        for ( int i = base; i < base + BATCH_SIZE; i++ ) {
                if ( !picked_[i] && selected_[i] == card ) {
                        picked_[i] = true;
                        owner_[i]  = player;
                        break;
                }
        }

        turn_++;

        if ( turn_ >= NUM_SELECTED ) {
                done_ = true;
                /* Count cards in [0, 7] per player. */
                int score[2] = { 0, 0 };
                for ( int i = 0; i < NUM_SELECTED; i++ ) {
                        if ( selected_[i] >= 0 && selected_[i] <= 7 ) {
                                score[owner_[i]]++;
                        }
                }
                if ( score[0] > score[1] )
                        winner_ = 0;
                else if ( score[1] > score[0] )
                        winner_ = 1;
                else
                        winner_ = 2;
        }
}

int
Game::CurrentPlayer() const
{
        if ( done_ ) return -1;
        return kDraftPlayer[turn_];
}

int
Game::Winner() const
{
        return winner_;
}

bool
Game::IsOver() const
{
        return done_;
}

void
Game::Show() const
{
        printf( "Turn %d/%d", turn_, NUM_SELECTED );
        if ( !done_ )
                printf( " | Player %d to pick", CurrentPlayer() );
        printf( "\n" );

        for ( int batch = 0; batch < 2; batch++ ) {
                int base = batch * BATCH_SIZE;
                printf( "  Batch %d:", batch );
                for ( int i = base; i < base + BATCH_SIZE; i++ ) {
                        if ( picked_[i] )
                                printf( " [%c:P%d]", CardChar( selected_[i] ),
                                        owner_[i] );
                        else
                                printf( " %c", CardChar( selected_[i] ) );
                }
                printf( "\n" );
        }

        if ( done_ ) {
                if ( winner_ == 2 )
                        printf( "Result: Tie\n" );
                else
                        printf( "Result: Player %d wins\n", winner_ );
        }
}

}  // namespace deep_wonders
