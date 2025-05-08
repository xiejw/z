#include <assert.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define ROWS 6
#define COLS 7

#define BIN_DATA_FILE    "tensor_data.bin"
#define MAX_DIM_LIMIT    5
#define MAX_TENSOR_LIMIT 128
#define MAX_ELE_DISPLAY  20

#define PANIC( msg )                 \
        do {                         \
                printf( "panic\n" ); \
                printf( msg );       \
                exit( -1 );          \
        } while ( 0 )

#define COL_ROW_TO_IDX( col, row ) ( ( row ) * COLS + ( col ) )

typedef float    f32;
typedef uint32_t u32;
typedef int32_t  i32;

typedef enum { NA, BLACK, WHITE } Color;

typedef struct {
        Color board[ROWS * COLS];
        Color next_player;
        Color nn_player;
} Game;

/* === Game related utilities ----------------------------------------------- */

/* Creates a new game and initializes nn player randomly. */
Game *
game_new( void )
{
        Game *g = calloc( 1, sizeof( *g ) );
        assert( g != NULL );
        g->next_player = BLACK;
        g->nn_player   = ( ( rand( ) < ( RAND_MAX >> 1 ) ) ? BLACK : WHITE );
        return g;
}

void
game_free( Game *g )
{
        if ( g == NULL ) return;
        free( g );
}

/* Display the board. */
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

/* Return the next legal row to place a stone in column (col) or -1 if no way.
 */
int
game_legal_row( Game *g, int col )
{
        Color *board = g->board;
        for ( int row = ROWS - 1; row >= 0; row-- ) {
                if ( board[COL_ROW_TO_IDX( col, row )] == NA ) return row;
        }
        return -1;
}

/* Return BLACK or WHITE if winner exists, 0 if tie, -1 if game is still
 * ongoing. */
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

int
policy_random_move( Game *g )
{
        while ( 1 ) {
                int col = rand( ) % COLS;
                int row = game_legal_row( g, col );
                if ( row != -1 ) return col;
        }
}

int
policy_human_move( Game *g )
{
        while ( 1 ) {
                int  col;
                char movec;
                printf( "Your move (1-7): " );
                scanf( " %c", &movec );
                col = movec - '1';  // Turn character into number.

                if ( col < 0 || col >= COLS ) {
                        printf(
                            "Invalid move! Must be ('1'-'7'). Try again.\n" );
                        continue;
                }

                int row = game_legal_row( g, col );
                if ( row == -1 ) {
                        printf( "Invalid move! Column is full. Try again.\n" );
                        continue;
                }
                return col;
        }
}

void
play_game( void )
{
        Game *g = game_new( );

        show_board( g );
        while ( 1 ) {
                int col, row;

                if ( g->next_player == g->nn_player ) {
                        col = policy_random_move( g );
                } else {
                        col = policy_human_move( g );
                }

                row = game_legal_row( g, col );
                if ( row == -1 ) {
                        printf( "invalid column during game %d\n", col );
                        PANIC( "sorry" );
                }

                printf( "Place new stone in column: %d\n", col + 1 );
                g->board[COL_ROW_TO_IDX( col, row )] = g->next_player;
                show_board( g );

                int winner = game_winner( g );
                switch ( winner ) {
                case (int)BLACK:
                        printf( "black wins\n" );
                        return;
                case (int)WHITE:
                        printf( "white wins\n" );
                        return;
                case 0:
                        printf( "tie\n" );
                        return;
                default:
                        g->next_player =
                            g->next_player == BLACK ? WHITE : BLACK;
                }
        }
        game_free( g );
}

int
main( void )
{
        srand( (unsigned)time( NULL ) );
        play_game( );
}
