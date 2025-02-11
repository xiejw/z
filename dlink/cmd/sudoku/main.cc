#include <cstring>
#include <vector>

#include <algos/dal.h>

#include <eve/base/log.h>

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
static const int PROLBEMS[][SIZE * SIZE] = {
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

// === --- Prototypes ------------------------------------------------------ ===
//
struct Option;  // Forward def.
using OptionVec = std::vector<Option>;
using DLTable   = eve::algos::dal::Table;

struct Option {
        int x;
        int y;
        int k;
};

static auto PrintProblem( const int *problem ) -> void;
static auto FindAllOptions( const int *problem, OptionVec &options ) -> size_t;
static auto HideColHeaders( DLTable &t, const int *problem ) -> void;
static auto InsertOptions( DLTable &t, const OptionVec &options ) -> void;
static auto PrintSolution( DLTable &t, const int *problem,
                           std::vector<std::size_t> &sols ) -> void;

/* For a digit k (1-based) in cell (i,j), together called placement, it will
 * cover four column headers ids:
 *
 * - pos{i,j}: Represent the cell position. All must have filled by a digit.
 * - row{i,k}: Represent the row with digit. Each row must have all digits.
 * - col{j,k}: Represent the column with digit. Each column must have all
 *             digits.
 * - box{x,k}: Represent the box with digit where x = 3 * floor(i/3) +
 *             floor(j/3).  Each box must have all digits.
 *
 * NOTE: There is an optimization in the code to use union to cast HeaderIds to
 * plain size_t array.
 */
struct HeaderIds {
        size_t pos;
        size_t row;
        size_t col;
        size_t box;
};
static_assert( std::is_trivial_v<HeaderIds> == true &&
               std::is_standard_layout_v<HeaderIds> == true );

static auto GenerateColHeaderIdsForPlacement( int i, int j, int k,
                                              HeaderIds *ids ) -> void;
// === --- main ------------------------------------------------------------ ===
//
auto
main( ) -> int
{
        // === --- Select and Print the Problem ---------------------------- ===
        //
        const int *problem = PROLBEMS[PID];
        logInfo( "Problem:\n" );
        PrintProblem( problem );

        // === --- Search Options ------------------------------------------ ===
        //
        // At most 9^3 options, try all digits in all cells.
        std::vector<Option> options{ };
        options.reserve( 9 * 9 * 9 );
        const size_t options_count = FindAllOptions( problem, options );

        // === --- Step 1: Prepare Dancing Links Table --------------------- ===
        //
        // For each option, we need to cover 4 headers (aka items), i.e., cell
        // position, row with digit column with digit, box with digit, and
        // digit. This is documented in details in
        // GenerateColHeaderIdsForPlacement for details.

        DLTable t{ /*n_col_heads=*/4 * 81,
                   /*n_options_total=*/4 * options_count };

        // === --- Step 2: Hide all Column Headers Filled Already ---------- ===
        //
        // The problem has few digits placed already. The column headers
        // corresponding to them should be uncovered (aka hidden).
        HideColHeaders( t, problem );

        // === --- Step 3: Append Options to the Dancing Links Table ------- ===
        //
        InsertOptions( t, options );

        // === --- Step 4: Print the Solution ------------------------------ ===
        //
        std::vector<std::size_t> sols{ };
        if ( t.SearchSolution( sols ) ) {
                PrintSolution( t, problem, sols );
        } else {
                printf( "No solution.\n" );
        }

        return 0;
}

// === --- Helper methods -------------------------------------------------- ===
//

// Print the Sudoku Problem on screen.
auto
PrintProblem( const int *problem ) -> void
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

// Seach all options that on (x,y) the digit k is allowed to be put there.
//
// The argument options must have enough capacity to hold all potential
// options
auto
FindAllOptions( const int *problem, std::vector<Option> &options ) -> size_t
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

                                options.push_back( { .x = x, .y = y, .k = k } );
                                total++;
                        not_a_option:
                                (void)0;
                        }
                }
        }

#undef POS

        if ( DEBUG ) {
                logInfo( "total options count %zu\n", total );
                logInfo( "top 10 options:\n" );
                for ( size_t i = 0; i < 10 && i < total; i++ ) {
                        logInfo( "  x %d, y %d, k %d\n", options[i].x,
                                 options[i].y, options[i].k );
                }
        }
        return total;
}

auto
HideColHeaders( DLTable &t, const int *problem ) -> void
{
        HeaderIds item_ids;
        for ( int x = 0; x < SIZE; x++ ) {
                int offset = x * SIZE;
                for ( int y = 0; y < SIZE; y++ ) {
                        int num = problem[offset + y];
                        if ( num == 0 ) continue;

                        GenerateColHeaderIdsForPlacement( x, y, num,
                                                          &item_ids );
                        t.CoverCol( item_ids.pos );
                        t.CoverCol( item_ids.row );
                        t.CoverCol( item_ids.col );
                        t.CoverCol( item_ids.box );
                }
        }
}

auto
InsertOptions( DLTable &t, const OptionVec &options ) -> void
{
        union ItemIds {
                HeaderIds in;
                size_t    out[4];
        } item_ids;
        for ( auto &opt : options ) {
                auto *o = &opt;
                GenerateColHeaderIdsForPlacement( o->x, o->y, o->k,
                                                  &item_ids.in );
                t.AppendOption( item_ids.out, (void *)o );
        }
}

auto
PrintSolution( DLTable &t, const int *problem, std::vector<std::size_t> &sols )
    -> void
{
        // Duplicate the problem as we are going to modify it.
        int solution[SIZE * SIZE];
        memcpy( solution, problem, sizeof( int ) * SIZE * SIZE );

        logInfo( "Found solution:\n" );
        size_t n = sols.size( );
        for ( size_t i = 0; i < n; i++ ) {
                Option *o = (Option *)t.GetNodeData( sols[i] );
                solution[o->x * SIZE + o->y] = o->k;
        }
        PrintProblem( solution );
}

auto
GenerateColHeaderIdsForPlacement( int i, int j, int k, HeaderIds *ids ) -> void
{
        int x      = 3 * ( i / 3 ) + ( j / 3 );
        int offset = 0;

        k = k - 1;  // k is 1 based.

        ids->pos =
            (size_t)( i * SIZE + j + offset + 1 );  // item id is 1 based.
        offset += SIZE * SIZE + 1;

        ids->row = (size_t)( i * SIZE + k + offset );
        offset += SIZE * SIZE;

        ids->col = (size_t)( j * SIZE + k + offset );
        offset += SIZE * SIZE;

        ids->box = (size_t)( x * SIZE + k + offset );
}
