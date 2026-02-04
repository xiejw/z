// See two_sat_solver.h
//
#include "test_macros.h"
#include "two_sat_solver.h"

namespace {
using namespace taocp;

FORGE_TEST( test_single_node_sat )
{
        // a || C(a)          C(a) -> C(a)
        TwoSatSolver s{ 1 };
        const size_t a = 0;
        s.AddKromClause( a, s.GetCompVarId( a ) );
        bool sat = s.CheckSatisfiability( );

        EXPECT_TRUE( sat, "true" );

        return NULL;
}

FORGE_TEST( test_single_node_constradict )
{
        // a || a               C(a) -> a
        // C(a) || C(a)         a -> C(a)
        TwoSatSolver s{ 1 };
        const size_t a = 0;
        s.AddKromClause( a, a );
        s.AddKromClause( s.GetCompVarId( a ), s.GetCompVarId( a ) );
        bool sat = s.CheckSatisfiability( );

        EXPECT_TRUE( !sat, "not sat" );

        return NULL;
}

FORGE_TEST( test_two_nodes )
{
        // C(a) || C(b)          a -> C(b)
        //   b  || C(a)          C(b) -> C(a)
        TwoSatSolver s{ 2 };
        const size_t a = 0;
        const size_t b = 1;
        s.AddKromClause( s.GetCompVarId( a ), s.GetCompVarId( b ) );
        s.AddKromClause( b, s.GetCompVarId( a ) );
        bool sat = s.CheckSatisfiability( );

        EXPECT_TRUE( sat, "true" );

        return NULL;
}

// Problem (37) on Vol 4A, Page 60. See also (39) on Page 61.
FORGE_TEST( test_comedians )
{
        TwoSatSolver s{ 7 };
        const size_t t = 0;
        const size_t u = 1;
        const size_t v = 2;
        const size_t w = 3;
        const size_t x = 4;
        const size_t y = 5;
        const size_t z = 6;

        s.AddKromClause( s.GetCompVarId( t ), s.GetCompVarId( w ) );
        s.AddKromClause( s.GetCompVarId( u ), s.GetCompVarId( z ) );
        s.AddKromClause( u, s.GetCompVarId( y ) );
        s.AddKromClause( u, z );
        s.AddKromClause( s.GetCompVarId( y ), z );
        s.AddKromClause( t, s.GetCompVarId( x ) );
        s.AddKromClause( t, z );
        s.AddKromClause( s.GetCompVarId( x ), z );
        s.AddKromClause( s.GetCompVarId( t ), s.GetCompVarId( z ) );
        s.AddKromClause( s.GetCompVarId( v ), y );
        s.AddKromClause( v, s.GetCompVarId( w ) );
        s.AddKromClause( v, s.GetCompVarId( y ) );
        s.AddKromClause( s.GetCompVarId( w ), s.GetCompVarId( y ) );
        s.AddKromClause( u, x );
        s.AddKromClause( s.GetCompVarId( u ), v );
        s.AddKromClause( s.GetCompVarId( v ), s.GetCompVarId( x ) );

        bool sat = s.CheckSatisfiability( );

        EXPECT_TRUE( !sat, "not sat" );

        return NULL;
}

}  // namespace

int
main( )
{
        forge::test_suite_run( );
}
