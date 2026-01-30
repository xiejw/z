#include "game.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace hermes {

Game *
game_new( void )
{
        Game *g = (Game *)calloc( 1, sizeof( *g ) );
        assert( g != NULL );
        g->next_player = BLACK;
        g->nn_player   = ( ( rand( ) % 2 == 0 ) ? BLACK : WHITE );
        return g;
}

void
game_free( Game *g )
{
        if ( g == NULL ) return;
        free( g );
}

Game *
game_dup_snapshot( Game *g )
{
        Game *ng = (Game *)malloc( sizeof( *ng ) );
        assert( ng != NULL );
        memcpy( ng, g, sizeof( *ng ) );
        return ng;
}

void
show_board( Game *g )
{
        printf( "  " );
        for ( int i = 0; i < COLS; i++ ) printf( " %d", i + 1 );
        printf( "\n" );

        for ( int y = 0; y < ROWS; y++ ) {
                printf( "%d ", y + 1 );
                for ( int x = 0; x < COLS; x++ ) {
                        Color c = g->board[COL_ROW_TO_IDX( x, y )];
                        if ( c == NA )
                                printf( " ." );
                        else if ( c == BLACK )
                                printf( " x" );
                        else if ( c == WHITE )
                                printf( " o" );
                }
                printf( "\n" );
        }
}

int
game_legal_row( Game *g, int col )
{
        Color *board = g->board;
        for ( int row = ROWS - 1; row >= 0; row-- ) {
                if ( board[COL_ROW_TO_IDX( col, row )] == NA ) return row;
        }
        return -1;
}

int
game_winner( Game *g )
{
        Color *board = g->board;
        assert( (int)BLACK > 0 );
        assert( (int)WHITE > 0 );
        /* Pass 1: check winner. */
        for ( int row = 0; row < ROWS; row++ ) {
                for ( int col = 0; col < COLS; col++ ) {
                        Color c = board[COL_ROW_TO_IDX( col, row )];
                        if ( c == NA ) continue;

                        /* Go right */
                        {
                                int i = 1;
                                for ( ; i <= 3; i++ ) {
                                        if ( col + i >= COLS ) break;
                                        int idx =
                                            COL_ROW_TO_IDX( col + i, row );
                                        if ( board[idx] != c ) break;
                                }

                                if ( i == 4 ) return (int)c;
                        }

                        /* Go down */
                        {
                                int i = 1;
                                for ( ; i <= 3; i++ ) {
                                        if ( row + i >= ROWS ) break;
                                        int idx =
                                            COL_ROW_TO_IDX( col, row + i );
                                        if ( board[idx] != c ) break;
                                }

                                if ( i == 4 ) return (int)c;
                        }

                        /* Go up right */
                        {
                                int i = 1;
                                for ( ; i <= 3; i++ ) {
                                        if ( row - i < 0 ) break;
                                        if ( col + i >= COLS ) break;
                                        int idx =
                                            COL_ROW_TO_IDX( col + i, row - i );
                                        if ( board[idx] != c ) break;
                                }

                                if ( i == 4 ) return (int)c;
                        }

                        /* Go down right */
                        {
                                int i = 1;
                                for ( ; i <= 3; i++ ) {
                                        if ( col + i >= COLS ) break;
                                        if ( row + i >= ROWS ) break;
                                        int idx =
                                            COL_ROW_TO_IDX( col + i, row + i );
                                        if ( board[idx] != c ) break;
                                }

                                if ( i == 4 ) return (int)c;
                        }
                }
        }

        /* Pass 2: check tie. */
        int tie = 1;
        for ( int x = 0; x < ROWS * COLS; x++ ) {
                if ( board[x] == NA ) {
                        tie = 0;
                        break;
                }
        }

        if ( tie ) return 0;

        /* Still ongoing */
        return -1;
}
}  // namespace hermes
