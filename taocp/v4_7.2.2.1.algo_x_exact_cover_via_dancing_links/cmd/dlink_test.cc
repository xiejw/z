#include "dlink.h"

#include "test_macros.h"

#include <algorithm>

#include <assert.h>
#include <stdio.h>
#include <string.h>

/* === --- Test Code -------------------------------------------------------- */

#define SetHeads3( h, a, b, c ) \
        ( ( h )[0] = ( a ), ( h )[1] = ( b ), ( h )[2] = ( c ) )
#define SetHeads2( h, a, b ) ( ( h )[0] = ( a ), ( h )[1] = ( b ) )

using namespace taocp;

int
main( void )
{
        // Exact cover problem: Cover all columns of a matrix exactly once.
        //
        //        1 2 3 4 5 6 7
        // row 1: 0 0 1 0 1 1 0    // 3 5 6
        // row 2: 1 0 0 1 0 0 1    // 1 4 7
        // row 3: 0 1 1 0 0 1 0    // 2 3 6
        // row 4: 1 0 0 1 0 0 0    // 1 4
        // row 5: 0 1 0 0 0 0 1    // 2 7
        // row 6: 0 0 0 1 1 0 1    // 4 5 7
        //
        // solution is
        // row 1 4 5

        DLinkTable tbl{ /*n_items=*/7,
                        /*n_options=*/6,
                        /*n_option_nodes=*/16 };

        struct OptionState {
                size_t      row_id;
                const char *names[6];       // Record the name.
                size_t      top_ids[6][3];  // At most 3 in each row.
        };

        const size_t SENT = 8;  // Invalid top id

        OptionState state{
            0,
            { "r1", "r2", "r3", "r4", "r5", "r6" },
            {
              /* row 0 */ { 3, 5, 6 },
              /* row 1 */ { 1, 4, 7 },
              /* row 2 */ { 2, 3, 6 },
              /* row 3 */ { 1, 4, SENT },
              /* row 4 */ { 2, 7, SENT },
              /* row 5 */ { 4, 5, 7 },
              }
        };

        tbl.AppendOptions(
            []( auto *user_data, size_t *total, size_t **top_ids ) {
                    auto  *state  = (OptionState *)user_data;
                    size_t row_id = state->row_id;
                    *top_ids      = state->top_ids[row_id];
                    *total        = state->top_ids[row_id][2] == SENT ? 2 : 3;
                    state->row_id++;
            },
            /*user_data=*/&state );

        size_t sols[6 + 1] = { };  // At most 6. +1 for SENT
        size_t sols_size   = 0;

        tbl.SearchSolutions(
            []( void *user_data, size_t solution_size,
                size_t *solution ) -> bool {
                    *( (size_t *)user_data ) = solution_size;
                    assert( solution[6] == 0 );
                    return true;
            },
            &sols_size, 7, sols );

        EXPECT_TRUE( sols_size == 3, "sols size" );
        assert( sols[0] != 0 );  // At least one solution

        std::sort( sols, sols + sols_size );

        EXPECT_TRUE( 1 == sols[0], "sol 0" );
        EXPECT_TRUE( 4 == sols[1], "sol 1" );
        EXPECT_TRUE( 5 == sols[2], "sol 2" );

        // Solution is 1 based. -1 to make it 0-based.
        EXPECT_TRUE( 0 == strcmp( "r1", state.names[sols[0] - 1] ), "name 0" );
        EXPECT_TRUE( 0 == strcmp( "r4", state.names[sols[1] - 1] ), "name 1" );
        EXPECT_TRUE( 0 == strcmp( "r5", state.names[sols[2] - 1] ), "name 2" );

        printf( "Test passed.\n" );
}
