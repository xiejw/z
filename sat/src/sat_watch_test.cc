#include <eve/testing/testing.h>

#include <algos/sat_watch.h>

using eve::algos::sat::C;
using eve::algos::sat::WatchSolver;

EVE_TEST( SatWatch, Simple )
{
        /* clauses
         * 1
         * 1 c2
         * 2 c3
         * 2 3
         *
         * answer is 1 2 any 3
         */
        WatchSolver sov{ /*num_literals=*/3, /*num_causes=*/4 };

        sov.EmitClause( { { ( 1 ) } } );
        sov.EmitClause( {
            { 1, C( 2 ) }
        } );
        sov.EmitClause( {
            { 2, C( 3 ) }
        } );
        sov.EmitClause( {
            { 2, 3 }
        } );

        auto res = sov.Search( );

        EVE_TEST_EXPECT( bool( res ) == true, "has answer" );
        EVE_TEST_EXPECT( res.value( ).size( ) == 3, "3 eles" );
        EVE_TEST_EXPECT( res.value( )[0] == 1, "[0] == 1" );
        EVE_TEST_EXPECT( res.value( )[1] == 2, "[1] == 2" );
        EVE_TEST_EXPECT( res.value( )[2] == C( 3 ), "[2] == c3" );
        EVE_TEST_PASS( );
}

EVE_TEST( SatWatch, Reverse )
{
        /* clauses
         * 3
         * c2 1
         * 3 2
         * c3 c1
         * 3 c1
         *
         * answer is c1 c2 3
         */
        WatchSolver sov{ /*num_literals=*/3, /*num_causes=*/5 };

        sov.EmitClause( { { 3 } } );
        sov.EmitClause( {
            { C( 2 ), 1 }
        } );
        sov.EmitClause( {
            { 3, 2 }
        } );
        sov.EmitClause( {
            { C( 3 ), C( 1 ) }
        } );
        sov.EmitClause( {
            { 3, C( 1 ) }
        } );

        auto res = sov.Search( );

        EVE_TEST_EXPECT( bool( res ) == true, "has answer" );
        EVE_TEST_EXPECT( res.value( ).size( ) == 3, "3 eles" );
        EVE_TEST_EXPECT( res.value( )[0] == C( 1 ), "[0] == c1" );
        EVE_TEST_EXPECT( res.value( )[1] == C( 2 ), "[1] == c2" );
        EVE_TEST_EXPECT( res.value( )[2] == 3, "[2] == 3" );
        EVE_TEST_PASS( );
}

EVE_TEST( SatWatch, NoResult )
{
        /* clauses
         * c1
         * 1 c2
         * 2 c3
         * 2 3
         *
         * answer is: no solution.
         */
        WatchSolver sov{ /*num_literals=*/3, /*num_causes=*/4 };

        sov.EmitClause( { { C( 1 ) } } );
        sov.EmitClause( {
            { 1, C( 2 ) }
        } );
        sov.EmitClause( {
            { 2, C( 3 ) }
        } );
        sov.EmitClause( {
            { 2, 3 }
        } );

        auto res = sov.Search( );

        EVE_TEST_EXPECT( bool( res ) == false, "no answer" );
        EVE_TEST_PASS( );
}
