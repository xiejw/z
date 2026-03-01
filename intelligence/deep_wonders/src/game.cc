// deep_wonders:v1
#include "game.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace deep_wonders {

Game *
game_new( void )
{
        Game *g = (Game *)calloc( 1, sizeof( *g ) );
        assert( g != NULL );
        g->current_player = 0;
        g->turn           = 0;
        g->done           = 0;
        g->winner         = -1;
        return g;
}

void
game_free( Game *g )
{
        if ( g == NULL ) return;
        free( g );
}

Game *
game_dup( Game *g )
{
        Game *ng = (Game *)malloc( sizeof( *ng ) );
        assert( ng != NULL );
        memcpy( ng, g, sizeof( *ng ) );
        return ng;
}

int
game_num_actions( Game * /*g*/ )
{
        return NUM_ACTIONS;
}

void
game_apply_action( Game *g, int action )
{
        assert( !g->done );
        assert( action >= 0 && action < NUM_ACTIONS );

        g->turn++;
        g->current_player = 1 - g->current_player;

        if ( g->turn >= MAX_TURNS ) {
                g->done = 1;
                /* Stub: player 0 wins if turn count is even, else player 1. */
                if ( g->turn % 2 == 0 )
                        g->winner = 0;
                else
                        g->winner = 1;
        }
}

int
game_winner( Game *g )
{
        return g->winner;
}

int
game_is_over( Game *g )
{
        return g->done;
}

void
show_game( Game *g )
{
        printf( "Turn: %d | Player: %d | ", g->turn, g->current_player );
        if ( g->done ) {
                if ( g->winner == 2 )
                        printf( "Result: Tie\n" );
                else
                        printf( "Result: Player %d wins\n", g->winner );
        } else {
                printf( "Status: Ongoing\n" );
        }
}

}  // namespace deep_wonders
