#include "dlink.h"

#include <stdio.h>
#include <string.h>

/* === --- Private helper utils --------------------------------------------- */

#define PANIC( )                     \
        do {                         \
                printf( "panic\n" ); \
                exit( -1 );          \
        } while ( 0 )

#define EXPECT_TRUE( eq_condition, msg )                 \
        do {                                             \
                if ( !( eq_condition ) ) {               \
                        printf( "Assertion failed.\n" ); \
                        printf( msg "\n" );              \
                        PANIC( );                        \
                }                                        \
        } while ( 0 )

/* === --- Test Code -------------------------------------------------------- */

#define SetHeads3( h, a, b, c ) \
        ( ( h )[0] = ( a ), ( h )[1] = ( b ), ( h )[2] = ( c ) )
#define SetHeads2( h, a, b ) ( ( h )[0] = ( a ), ( h )[1] = ( b ) )

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

        struct dlink_tbl *tbl = dlink_new( /*n_items=*/7, /*n_opt_nodes=*/16 );

        size_t heads3[3];
        size_t heads2[2];

        // Row 1
        SetHeads3( heads3, 3, 5, 6 );
        dlink_append_opt( tbl, 3, heads3, (void *)"r1" );
        // Row 2
        SetHeads3( heads3, 1, 4, 7 );
        dlink_append_opt( tbl, 3, heads3, (void *)"r2" );
        // Row 3
        SetHeads3( heads3, 2, 3, 6 );
        dlink_append_opt( tbl, 3, heads3, (void *)"r3" );
        // Row 4
        SetHeads2( heads2, 1, 4 );
        dlink_append_opt( tbl, 2, heads2, (void *)"r4" );
        // Row 5
        SetHeads2( heads2, 2, 7 );
        dlink_append_opt( tbl, 2, heads2, (void *)"r5" );
        // Row 6
        SetHeads3( heads3, 4, 5, 7 );
        dlink_append_opt( tbl, 3, heads3, (void *)"r3" );

        size_t  sols[6]; /* At most 6. */
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
            0 == strcmp( "r4", (char *)dlink_get_node_data( tbl, sols[0] ) ),
            "sol 0" );
        EXPECT_TRUE(
            0 == strcmp( "r5", (char *)dlink_get_node_data( tbl, sols[1] ) ),
            "sol 1" );
        EXPECT_TRUE(
            0 == strcmp( "r1", (char *)dlink_get_node_data( tbl, sols[2] ) ),
            "sol 2" );

        dlink_free( tbl );
        printf( "Test passed.\n" );
}
