#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "game.h"
#include "log.h"
#include "mcts.h"
#include "nn.h"

using namespace deep_wonders;

#ifndef MCTS_ITER_CNT
#define MCTS_ITER_CNT 100
#endif

#ifndef NUM_GAMES
#define NUM_GAMES 10
#endif

void
self_play_one_game( NN *nn, int game_id )
{
        Game *g = new Game();

        while ( !g->IsOver() ) {
                int action = mcts_search( g, nn, MCTS_ITER_CNT );
                g->ApplyAction( action );
        }

        int winner = g->Winner();
        if ( winner == 2 )
                printf( "Game %d: Tie\n", game_id );
        else
                printf( "Game %d: Player %d wins\n", game_id, winner );

        delete g;
}

int
main( void )
{
        srand( (unsigned)time( NULL ) );
        NN *nn = nn_new( );

        INFO( "Running %d self-play games.\n", NUM_GAMES );
        for ( int i = 0; i < NUM_GAMES; i++ ) {
                self_play_one_game( nn, i );
        }
        INFO( "Self-play complete.\n" );

        nn_free( nn );
}
