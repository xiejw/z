#include <testing/testing.h>

#include <algos/dal.h>

#include <string.h>

#define setHeads3( h, a, b, c ) \
        ( ( h )[0] = ( a ), ( h )[1] = ( b ), ( h )[2] = ( c ) )
#define setHeads2( h, a, b ) ( ( h )[0] = ( a ), ( h )[1] = ( b ) )

static char *
test_matrix_cover( )
{
        struct dal_table *t = dalNew( 1 + 23 );
        // column exact cover problem
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

        // items
        dalAllocColHeads( t, /*num_colheads=*/7 );
        // row 1
        size_t heads[3];
        setHeads3( heads, 3, 5, 6 );
        dalAppendOption( t, 3, heads, "r1" );
        // row 2
        setHeads3( heads, 1, 4, 7 );
        dalAppendOption( t, 3, heads, "r2" );
        // row 3
        setHeads3( heads, 2, 3, 6 );
        dalAppendOption( t, 3, heads, "r3" );
        // row 4
        setHeads2( heads, 1, 4 );
        dalAppendOption( t, 2, heads, "r4" );
        // row 5
        setHeads2( heads, 2, 7 );
        dalAppendOption( t, 2, heads, "r5" );
        // row 6
        setHeads3( heads, 4, 5, 7 );
        dalAppendOption( t, 3, heads, "r6" );

        ASSERT_TRUE( "table item size", 7 == t->num_colheads );
        ASSERT_TRUE( "table size", 24 == t->num_nodes );
        ASSERT_TRUE( "table size", 24 == vecSize( t->nodes ) );
        ASSERT_TRUE( "table cap", 24 == vecCap( t->nodes ) );

        // print(h, t->num_nodes, t->num_colheads);

        vec_t( size_t ) sols = vecNew( );
        vecReserve( &sols, 4 );

        ASSERT_TRUE( "found sol", 1 == dalSearchSolution( t, sols ) );
        ASSERT_TRUE( "found sol", 3 == vecSize( sols ) );
        ASSERT_TRUE( "sol 0", 17 == sols[0] );
        ASSERT_TRUE( "sol 1", 19 == sols[1] );
        ASSERT_TRUE( "sol 2", 8 == sols[2] );
        ASSERT_TRUE( "sol 0", 0 == strcmp( "r4", dalNodeData( t, sols[0] ) ) );
        ASSERT_TRUE( "sol 1", 0 == strcmp( "r5", dalNodeData( t, sols[1] ) ) );
        ASSERT_TRUE( "sol 2", 0 == strcmp( "r1", dalNodeData( t, sols[2] ) ) );

        vecFree( sols );
        dalFree( t );
        return NULL;
}

DECLARE_TEST_SUITE( algos_dal )
{
        RUN_TEST( test_matrix_cover );
        return NULL;
}
