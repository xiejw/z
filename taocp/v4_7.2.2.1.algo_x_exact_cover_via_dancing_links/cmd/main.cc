// Solve Sudoku via Dancing Links.
//
// See dlink.h
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "dlink.h"

namespace {

#define SIZE 9

#ifndef DEBUG
#define DEBUG 1
#endif

#ifndef PID  // Problem ID
#define PID 1
#endif

// === --- Predefined Problems --------------------------------------------- ===
//
// clang-format off
 const int PROLBEMS[][SIZE * SIZE] = {
    // Problem ID: 0
    //
    // From The Art of Computer Programming, Vol 4, Dancing Links.
    {
        0, 0, 3,  0, 1, 0,  0, 0, 0,
        4, 1, 5,  0, 0, 0,  0, 9, 0,
        2, 0, 6,  5, 0, 0,  3, 0, 0,

        5, 0, 0,  0, 8, 0,  0, 0, 9,
        0, 7, 0,  9, 0, 0,  0, 3, 2,
        0, 3, 8,  0, 0, 4,  0, 6, 0,

        0, 0, 0,  2, 6, 0,  4, 0, 3,
        0, 0, 0,  3, 0, 0,  0, 0, 8,
        3, 2, 0,  0, 0, 7,  9, 5, 0,
    },

    // Problem ID: 1
    //
    // The World's Hardest Sudoku.
    //
    // https://gizmodo.com/can-you-solve-the-10-hardest-logic-puzzles-ever-created-1064112665
    {
        8, 0, 0,  0, 0, 0,  0, 0, 0,
        0, 0, 3,  6, 0, 0,  0, 0, 0,
        0, 7, 0,  0, 9, 0,  2, 0, 0,

        0, 5, 0,  0, 0, 7,  0, 0, 0,
        0, 0, 0,  0, 4, 5,  7, 0, 0,
        0, 0, 0,  1, 0, 0,  0, 3, 0,

        0, 0, 1,  0, 0, 0,  0, 6, 8,
        0, 0, 8,  5, 0, 0,  0, 1, 0,
        0, 9, 0,  0, 0, 0,  4, 0, 0,
    },
};
// clang-format on

// === --- Prototypes of Helper Methods ------------------------------------ ===
//

// An option is to put a digit k into cell (x,y).
struct opt {
        int x;
        int y;
        int k;
};

/* For a digit k (1-based) in cell (i,j), together called option, it will
 * cover four item ids:
 *
 * - pos{i,j}: Represent the cell position. All must have filled by a digit.
 * - row{i,k}: Represent the row with digit. Each row must have all digits.
 * - col{j,k}: Represent the column with digit. Each column must have all
 *             digits.
 * - box{x,k}: Represent the box with digit where x = 3 * floor(i/3) +
 *             floor(j/3).  Each box must have all digits.
 *
 * NOTE: There is an optimization in the code to use union to cast item_ids to
 * plain size_t array.
 */
struct item_ids {
        size_t pos;
        size_t row;
        size_t col;
        size_t box;
};

// === --- Helper Methods -------------------------------------------------- ===
//

// Print the Sudoku Problem on screen.
void
PrintProblem( const int *problem )
{
        // header
        printf( "+-----+-----+-----+\n" );
        for ( int x = 0; x < SIZE; x++ ) {
                int offset = x * SIZE;
                printf( "|" );
                for ( int y = 0; y < SIZE; y++ ) {
                        int num = problem[offset + y];
                        if ( num == 0 )
                                printf( " " );
                        else
                                printf( "%d", problem[offset + y] );

                        if ( ( y + 1 ) % 3 != 0 )
                                printf( " " );
                        else
                                printf( "|" );
                }
                printf( "\n" );
                if ( ( x + 1 ) % 3 == 0 ) printf( "+-----+-----+-----+\n" );
        }
}

/* Seach all options that on (x,y) the digit k is allowed to be put there.
 *
 * The argument opts must have enough capacity to hold all potential options.
 *
 * Returns the option count.
 */
size_t
FindAllOpts( const int *problem, struct opt *opts )
{
        size_t total = 0;

#define POS( x, y ) ( ( x ) * SIZE + ( y ) )

        for ( int x = 0; x < SIZE; x++ ) {
                const int offset = x * SIZE;

                for ( int y = 0; y < SIZE; y++ ) {
                        if ( problem[offset + y] > 0 ) continue;  // prefilled.

                        int box_x = ( x / 3 ) * 3;
                        int box_y = ( y / 3 ) * 3;

                        for ( int k = 1; k <= SIZE; k++ ) {
                                // Search row
                                for ( int j = 0; j < SIZE; j++ ) {
                                        if ( problem[j + offset] == k ) {
                                                goto not_a_option;
                                        }
                                }

                                // Search column
                                for ( int j = 0; j < SIZE; j++ ) {
                                        if ( problem[POS( j, y )] == k ) {
                                                goto not_a_option;
                                        }
                                }

                                // Search box. Static unroll
                                if ( problem[POS( box_x, box_y )] == k ||
                                     problem[POS( box_x, box_y + 1 )] == k ||
                                     problem[POS( box_x, box_y + 2 )] == k ||
                                     problem[POS( box_x + 1, box_y )] == k ||
                                     problem[POS( box_x + 1, box_y + 1 )] ==
                                         k ||
                                     problem[POS( box_x + 1, box_y + 2 )] ==
                                         k ||
                                     problem[POS( box_x + 2, box_y )] == k ||
                                     problem[POS( box_x + 2, box_y + 1 )] ==
                                         k ||
                                     problem[POS( box_x + 2, box_y + 2 )] ==
                                         k ) {
                                        goto not_a_option;
                                }

                                {
                                        struct opt opt;
                                        opt.x         = x;
                                        opt.y         = y;
                                        opt.k         = k;
                                        opts[total++] = opt;
                                }
                        not_a_option:
                                (void)0;
                        }
                }
        }

#undef POS

        if ( DEBUG ) {
                printf( "total options count %zu\n", total );
                printf( "top 10 options:\n" );
                for ( size_t i = 0; i < 10 && i < total; i++ ) {
                        printf( "  x %d, y %d, k %d\n", opts[i].x, opts[i].y,
                                opts[i].k );
                }
        }
        return total;
}

struct item_ids
GenerateItemIdsForOption( int i, int j, int k )
{
        struct item_ids ids;
        int             x      = 3 * ( i / 3 ) + ( j / 3 );
        int             offset = 0;

        k = k - 1;  // k is 1 based.

        ids.pos = (size_t)( i * SIZE + j + offset + 1 );  // item id is 1
        offset += SIZE * SIZE + 1;

        ids.row = (size_t)( i * SIZE + k + offset );
        offset += SIZE * SIZE;

        ids.col = (size_t)( j * SIZE + k + offset );
        offset += SIZE * SIZE;

        ids.box = (size_t)( x * SIZE + k + offset );
        return ids;
}

void
CoverItems( taocp::DLinkTable *t, const int *problem )
{
        struct item_ids item_ids;
        for ( int x = 0; x < SIZE; x++ ) {
                int offset = x * SIZE;
                for ( int y = 0; y < SIZE; y++ ) {
                        int num = problem[offset + y];
                        if ( num == 0 ) continue;

                        item_ids = GenerateItemIdsForOption( x, y, num );
                        t->CoverItem( item_ids.pos );
                        t->CoverItem( item_ids.row );
                        t->CoverItem( item_ids.col );
                        t->CoverItem( item_ids.box );
                }
        }
}

void
InsertOptions( taocp::DLinkTable *tbl, size_t opt_count, struct opt *opts )
{
        union item_ids_with_arr {
                struct item_ids in;
                size_t          out[4];
        };

        struct AppendState {
                size_t            opt_id;
                size_t            opt_count;
                struct opt       *opt_array;
                item_ids_with_arr item_ids;
        };

        AppendState state{ };
        state.opt_id    = 0;
        state.opt_count = opt_count;
        state.opt_array = opts;

        auto append_fn = []( void *user_data, size_t *option_node_size,
                             size_t **option_node_top_ids ) {
                auto *state = (AppendState *)user_data;

                size_t i = state->opt_id;
                assert( i < state->opt_count );
                struct opt *o = &state->opt_array[i];

                state->item_ids.in =
                    GenerateItemIdsForOption( o->x, o->y, o->k );
                *option_node_size    = 4;
                *option_node_top_ids = state->item_ids.out;
                state->opt_id++;
        };

        tbl->AppendOptions( append_fn, &state );
}

void
PrintSolution( taocp::DLinkTable *tbl, const int *problem, struct opt *opts,
               size_t num_sol, size_t *sol )
{
        (void)tbl;  // Unused;

        // Duplicate the problem as we are going to modify it.
        int solved_problem[SIZE * SIZE];
        memcpy( solved_problem, problem, sizeof( int ) * SIZE * SIZE );

        for ( size_t i = 0; i < num_sol; i++ ) {
                size_t      opt_id                 = sol[i];
                struct opt *o                      = &opts[opt_id];
                solved_problem[o->x * SIZE + o->y] = o->k;
        }
        PrintProblem( solved_problem );
}
}  // namespace

// === --- main ------------------------------------------------------------ ===
//
int
main( void )
{
        // === --- Select and Print the Problem ---------------------------- ===
        //
        const int *problem = PROLBEMS[PID];
        printf( "Problem:\n" );
        PrintProblem( problem );

        // === --- Search Options ------------------------------------------ ===
        //
        // At most 9^3 options, try all digits in all cells.
        struct opt   opts[9 * 9 * 9];
        const size_t opts_count = FindAllOpts( problem, opts );

        // === --- Step 1: Prepare Dancing Links Table --------------------- ===
        //
        // For each option, we need to cover 4 items,
        // - row with digit
        // - column with digit,
        // - box with digit, and
        // - digit.
        //
        // This is documented in details in GenerateItemIdsForOption for
        // details.
        taocp::DLinkTable tbl{ /*n_items=*/4 * 81, /*n_options=*/opts_count,
                               /*n_option_nodes=*/4 * opts_count };

        // === --- Step 2: Hide all Column Headers Filled Already ---------- ===
        //
        // The problem has few digits placed already. The item ids
        // corresponding to them should be covered (aka hidden).
        CoverItems( &tbl, problem );

        // === --- Step 3: Append Options to the Dancing Links Table ------- ===
        //
        InsertOptions( &tbl, opts_count, opts );

        // === --- Step 4: Print the Solution ------------------------------ ===

        size_t sol[9 * 9];
        size_t num_sol  = 0;
        auto   visit_fn = []( void *user_data, size_t solution_size,
                            size_t *user_solution ) {
                (void)user_solution;  // Unused

                *( (size_t *)user_data ) = solution_size;
                assert( solution_size <= 9 * 9 );
                return true;  // Stop immediately.
        };

        tbl.SearchSolutions( visit_fn, &num_sol, 9 * 9, sol );

        if ( num_sol > 0 ) {
                assert( num_sol <= 9 * 9 );
                printf( "Found solution:\n" );
                PrintSolution( &tbl, problem, opts, num_sol, sol );
                return 0;
        }

        printf( "No solution.\n" );
        return -1;
}
