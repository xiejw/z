// forge:skip
#include "sat_cdcl.h"
#include "sat_literal.h"
#include "test_macros.h"

using taocp::C;
using taocp::CDCLSolver;

FORGE_TEST( simple )
{
        /* clauses
         * 1
         * 1 c2
         * 2 c3
         * 2 3
         *
         * answer is 1 2 any 3
         */
        // WatchSolver sov{ /*num_literals=*/3, /*num_causes=*/4 };

        // sov.EmitClause( { ( 1 ) } );
        // sov.EmitClause( { 1, C( 2 ) } );
        // sov.EmitClause( { 2, C( 3 ) } );
        // sov.EmitClause( { 2, 3 } );

        // auto res = sov.SearchOneSolution( );

        // EXPECT_TRUE( bool( res ) == true, "has answer" );
        // EXPECT_TRUE( res.value( ).size( ) == 3, "3 eles" );
        // EXPECT_TRUE( res.value( )[0] == 1, "[0] == 1" );
        // EXPECT_TRUE( res.value( )[1] == 2, "[1] == 2" );
        // EXPECT_TRUE( res.value( )[2] == C( 3 ), "[2] == c3" );
}

int
main( )
{
        ::forge::test_suite_run( );
        return 0;
}
