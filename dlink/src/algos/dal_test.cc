#include <eve/testing/testing.h>

#include <cstring>

#include <algos/dal.h>

#define SetHeads3( h, a, b, c ) \
        ( ( h )[0] = ( a ), ( h )[1] = ( b ), ( h )[2] = ( c ) )
#define SetHeads2( h, a, b ) ( ( h )[0] = ( a ), ( h )[1] = ( b ) )

using DLTable = eve::algos::dal::Table;
EVE_TEST( DLTable, MatrixCover )
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

        DLTable t{ /*n_col_heads=*/7, /*n_options_total=*/16 };

        size_t heads[3];
        size_t heads2[2];

        // Row 1
        SetHeads3( heads, 3, 5, 6 );
        t.AppendOption( heads, (void *)"r1" );
        // Row 2
        SetHeads3( heads, 1, 4, 7 );
        t.AppendOption( heads, (void *)"r2" );
        // Row 3
        SetHeads3( heads, 2, 3, 6 );
        t.AppendOption( heads, (void *)"r3" );
        // Row 4
        SetHeads2( heads2, 1, 4 );
        t.AppendOption( heads2, (void *)"r4" );
        // Row 5
        SetHeads2( heads2, 2, 7 );
        t.AppendOption( heads2, (void *)"r5" );
        // Row 6
        SetHeads3( heads, 4, 5, 7 );
        t.AppendOption( heads, (void *)"r3" );

        auto opt_sol = t.SearchSolution( );
        EVE_TEST_EXPECT( bool( opt_sol ), "found sol" );
        EVE_TEST_EXPECT( 3 == opt_sol->size( ), "found sol" );

        // Check header ids.
        auto &sol = opt_sol.value( );
        EVE_TEST_EXPECT( 17 == sol[0], "sol 0" );
        EVE_TEST_EXPECT( 19 == sol[1], "sol 1" );
        EVE_TEST_EXPECT( 8 == sol[2], "sol 2" );

        // Check option data.
        EVE_TEST_EXPECT( 0 == strcmp( "r4", (char *)t.GetNodeData( sol[0] ) ),
                         "sol 0" );
        EVE_TEST_EXPECT( 0 == strcmp( "r5", (char *)t.GetNodeData( sol[1] ) ),
                         "sol 1" );
        EVE_TEST_EXPECT( 0 == strcmp( "r1", (char *)t.GetNodeData( sol[2] ) ),
                         "sol 2" );

        EVE_TEST_PASS( );
}
