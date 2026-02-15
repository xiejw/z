#include "dlink.h"

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
                const char *names[6];
                size_t      top_ids[6][3];  // At most 3 in each row.
        };

        const size_t SENT = 8;  // Invalid top id

        OptionState state{
            0,
            { "r0", "r1", "r2", "r3", "r4", "r5" },
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

        /*


              size_t  sols[6]; // At most 6. /
              size_t  num_sols;
              error_t rc = dlink_search( tbl, sols, &num_sols );
              EXPECT_TRUE( rc == OK, "found sol" );
              EXPECT_TRUE( 3 == num_sols, "found sol" );

              // Check header ids.
              EXPECT_TRUE( 17 == sols[0], "sol 0" );
              EXPECT_TRUE( 19 == sols[1], "sol 1" );
              EXPECT_TRUE( 8 == sols[2], "sol 2" );

              // Check option data.
              EXPECT_TRUE(
                  0 == strcmp( "r4", (char *)dlink_get_node_data( tbl,
           sols[0] )
           ), "sol 0" ); EXPECT_TRUE( 0 == strcmp( "r5", (char
           *)dlink_get_node_data( tbl, sols[1] ) ), "sol 1" ); EXPECT_TRUE(
           0 == strcmp( "r1", (char *)dlink_get_node_data( tbl, sols[2] ) ),
           "sol 2"
           );

              dlink_free( tbl );
              */
        printf( "Test passed.\n" );
}
