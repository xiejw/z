// See two_sat_solver.h
//
// forge:skip
#include "test_macros.h"
#include "two_sat_solver.h"

namespace {
using namespace taocp;

FORGE_TEST( test_single_node_sat )
{
        // a || C(a)          C(a) -> C(a)
        TwoSatSolver s{ 1 };
        const size_t a = 0;
        s.AddKromClause( a, s.GetComplementId( a ) );
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
        s.AddKromClause( s.GetComplementId( a ), s.GetComplementId( a ) );
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
        s.AddKromClause( s.GetComplementId( a ), s.GetComplementId( b ) );
        s.AddKromClause( b, s.GetComplementId( a ) );
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

        s.AddKromClause( s.GetComplementId( t ), s.GetComplementId( w ) );
        s.AddKromClause( s.GetComplementId( u ), s.GetComplementId( z ) );
        s.AddKromClause( u, s.GetComplementId( y ) );
        s.AddKromClause( u, z );
        s.AddKromClause( s.GetComplementId( y ), z );
        s.AddKromClause( t, s.GetComplementId( x ) );
        s.AddKromClause( t, z );
        s.AddKromClause( s.GetComplementId( x ), z );
        s.AddKromClause( s.GetComplementId( t ), s.GetComplementId( z ) );
        s.AddKromClause( s.GetComplementId( v ), y );
        s.AddKromClause( v, s.GetComplementId( w ) );
        s.AddKromClause( v, s.GetComplementId( y ) );
        s.AddKromClause( s.GetComplementId( w ), s.GetComplementId( y ) );
        s.AddKromClause( u, x );
        s.AddKromClause( s.GetComplementId( u ), v );
        s.AddKromClause( s.GetComplementId( v ), s.GetComplementId( x ) );

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
