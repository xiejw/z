// deep_wonders:v1
#include "game.h"

#include <assert.h>
#include <stdio.h>

namespace deep_wonders {

Game::Game()
        : current_player_( 0 ), turn_( 0 ), done_( false ), winner_( -1 )
{
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

void
Game::ApplyAction( int action )
{
        assert( !done_ );
        assert( action >= 0 && action < NUM_ACTIONS );

        turn_++;
        current_player_ = 1 - current_player_;

        if ( turn_ >= MAX_TURNS ) {
                done_ = true;
                /* Stub: player 0 wins if turn count is even, else player 1. */
                if ( turn_ % 2 == 0 )
                        winner_ = 0;
                else
                        winner_ = 1;
        }
}

int
Game::CurrentPlayer() const
{
        return current_player_;
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
        printf( "Turn: %d | Player: %d | ", turn_, current_player_ );
        if ( done_ ) {
                if ( winner_ == 2 )
                        printf( "Result: Tie\n" );
                else
                        printf( "Result: Player %d wins\n", winner_ );
        } else {
                printf( "Status: Ongoing\n" );
        }
}

}  // namespace deep_wonders
