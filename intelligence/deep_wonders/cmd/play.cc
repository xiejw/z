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

int
policy_human_move( Game *g )
{
        int n = game_num_actions( g );
        while ( 1 ) {
                char movec;
                printf( "[Player %d] Your move (0-%d): ",
                        g->current_player, n - 1 );
                if ( EOF == scanf( " %c", &movec ) ) {
                        PANIC( "eof, unexpected.\n" );
                }
                int action = movec - '0';
                if ( action < 0 || action >= n ) {
                        printf( "Invalid move! Must be 0-%d. Try again.\n",
                                n - 1 );
                        continue;
                }
                return action;
        }
}

int
policy_ai_move( Game *g, NN *nn )
{
        return mcts_search( g, nn, MCTS_ITER_CNT );
}

void
play_game( NN *nn )
{
        Game *g         = game_new( );
        int   ai_player = rand( ) % 2;

        INFO( "You are Player %d. AI is Player %d.\n", 1 - ai_player,
              ai_player );

        show_game( g );
        while ( !game_is_over( g ) ) {
                int action;
                if ( g->current_player == ai_player ) {
                        action = policy_ai_move( g, nn );
                        printf( "AI plays action %d\n", action );
                } else {
                        action = policy_human_move( g );
                }

                game_apply_action( g, action );
                show_game( g );
        }

        int winner = game_winner( g );
        if ( winner == 2 )
                printf( "Game over: Tie!\n" );
        else if ( winner == ai_player )
                printf( "Game over: AI wins!\n" );
        else
                printf( "Game over: You win!\n" );

        game_free( g );
}

int
main( void )
{
        srand( (unsigned)time( NULL ) );
        NN *nn = nn_new( );
        play_game( nn );
        nn_free( nn );
}
