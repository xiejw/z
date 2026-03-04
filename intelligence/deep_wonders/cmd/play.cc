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
        int  legal[NUM_ACTIONS];
        int  n = g->LegalActions( legal );

        /* Show available cards. */
        printf( "  Available:" );
        for ( int i = 0; i < n; i++ ) {
                int card = ActionCard( legal[i] );
                if ( card < 10 )
                        printf( " %d", card );
                else
                        printf( " %c", (char)( 'a' + card - 10 ) );
        }
        printf( "\n" );

        while ( 1 ) {
                char movec;
                printf( "[Player %d] Pick a card: ", g->CurrentPlayer() );
                if ( EOF == scanf( " %c", &movec ) ) {
                        PANIC( "eof, unexpected.\n" );
                }

                int card;
                if ( movec >= '0' && movec <= '9' )
                        card = movec - '0';
                else if ( movec >= 'a' && movec <= 'b' )
                        card = 10 + ( movec - 'a' );
                else {
                        printf( "Invalid input. Use 0-9 or a-b.\n" );
                        continue;
                }

                int action = ActionEncode( card, OP_SELECT );
                if ( !g->IsLegalAction( action ) ) {
                        printf( "Card not available. Try again.\n" );
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
        Game *g         = new Game();
        int   ai_player = rand( ) % 2;

        INFO( "You are Player %d. AI is Player %d.\n", 1 - ai_player,
              ai_player );

        g->Show();
        while ( !g->IsOver() ) {
                int action;
                if ( g->CurrentPlayer() == ai_player ) {
                        action = policy_ai_move( g, nn );
                        int card = ActionCard( action );
                        if ( card < 10 )
                                printf( "AI picks card %d\n", card );
                        else
                                printf( "AI picks card %c\n",
                                        (char)( 'a' + card - 10 ) );
                } else {
                        action = policy_human_move( g );
                }

                g->ApplyAction( action );
                g->Show();
        }

        int winner = g->Winner();
        if ( winner == 2 )
                printf( "Game over: Tie!\n" );
        else if ( winner == ai_player )
                printf( "Game over: AI wins!\n" );
        else
                printf( "Game over: You win!\n" );

        delete g;
}

int
main( void )
{
        srand( (unsigned)time( NULL ) );
        NN *nn = nn_new( );
        play_game( nn );
        nn_free( nn );
}
