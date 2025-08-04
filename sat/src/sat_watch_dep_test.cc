#include <eve/testing/testing.h>

#include <algos/sat_watch.h>

using eve::algos::sat::C;
using eve::algos::sat::WatchSolver;

EVE_TEST( SatWatch, DepResolve )
{
        /* See source
         * https://borretti.me/article/dependency-resolution-made-simple
         *
         * Assignments (* is final selection)
         *  1 sql:0
         *  2 sql:1
         *  3 sql:2*
         *
         *  4 threads:0
         *  5 threads:1
         *  6 threads:2*
         *
         *  7 http:0
         *  8 http:1
         *  9 http:2
         * 10 http:3
         * 11 http:4*
         *
         * 12 stdlib:0
         * 13 stdlib:1
         * 14 stdlib:2
         * 15 stdlib:3
         * 16 stdlib:4*
         */
        WatchSolver sov{ /*num_literals=*/16,
                         /*num_causes=*/3 * 2 + 10 * 2 + 16 };

        // clang-format off
        /* === --- Consistency ------------------------------------------ === */
        sov.EmitClause( { { C( 1 ) , C( 2 ) } } );
        sov.EmitClause( { { C( 1 ) , C( 3 ) } } );
        sov.EmitClause( { { C( 2 ) , C( 3 ) } } );

        sov.EmitClause( { { C( 4 ) , C( 5 ) } } );
        sov.EmitClause( { { C( 4 ) , C( 6 ) } } );
        sov.EmitClause( { { C( 5 ) , C( 6 ) } } );

        sov.EmitClause( { { C( 7 ) , C( 8 ) } } );
        sov.EmitClause( { { C( 7 ) , C( 9 ) } } );
        sov.EmitClause( { { C( 7 ) , C( 10 ) } } );
        sov.EmitClause( { { C( 7 ) , C( 11 ) } } );
        sov.EmitClause( { { C( 8 ) , C( 9 ) } } );
        sov.EmitClause( { { C( 8 ) , C( 10 ) } } );
        sov.EmitClause( { { C( 8 ) , C( 11 ) } } );
        sov.EmitClause( { { C( 9 ) , C( 10 ) } } );
        sov.EmitClause( { { C( 9 ) , C( 11 ) } } );
        sov.EmitClause( { { C( 10 ) , C( 11 ) } } );

        sov.EmitClause( { { C( 12 ) , C( 13 ) } } );
        sov.EmitClause( { { C( 12 ) , C( 14 ) } } );
        sov.EmitClause( { { C( 12 ) , C( 15 ) } } );
        sov.EmitClause( { { C( 12 ) , C( 16 ) } } );
        sov.EmitClause( { { C( 13 ) , C( 14 ) } } );
        sov.EmitClause( { { C( 13 ) , C( 15 ) } } );
        sov.EmitClause( { { C( 13 ) , C( 16 ) } } );
        sov.EmitClause( { { C( 14 ) , C( 15 ) } } );
        sov.EmitClause( { { C( 14 ) , C( 16 ) } } );
        sov.EmitClause( { { C( 15 ) , C( 16 ) } } );

        /* === --- Dependencies ----------------------------------------- === */
        sov.EmitClause( { { C( 2 ), 5 } } );
        sov.EmitClause( { { C( 2 ), 13, 14, 15, 16 } } );

        sov.EmitClause( { { C( 3 ), 5, 6 } } );
        sov.EmitClause( { { C( 3 ), 14, 15, 16 } } );

        sov.EmitClause( { { C( 4 ), 14, 15, 16 } } );
        sov.EmitClause( { { C( 5 ), 14, 15, 16 } } );
        sov.EmitClause( { { C( 6 ), 15, 16 } } );

        sov.EmitClause( { { C( 7 ), 12, 13, 14, 15 } } );
        sov.EmitClause( { { C( 8 ), 12, 13, 14, 15 } } );
        sov.EmitClause( { { C( 9 ), 13, 14, 15, 16 } } );
        sov.EmitClause( { { C( 10 ), 14, 15, 16 } } );
        sov.EmitClause( { { C( 11 ), 15, 16 } } );

        /* === --- Root  ------------------------------------------------ === */
        sov.EmitClause( { { 3 } } );
        sov.EmitClause( { { 6 } } );
        sov.EmitClause( { { 10, 11 } } );
        sov.EmitClause( { { 16 } } );

        // clang-format on
        auto res = sov.Search( );

        EVE_TEST_EXPECT( bool( res ) == true, "has answer" );
        EVE_TEST_EXPECT( res.value( ).size( ) == 16, "16 eles" );
        for ( size_t i = 0; i < res.value( ).size( ); i++ ) {
                switch ( i + 1 ) {
                case 3:
                        // passthrough
                case 6:
                        // passthrough
                case 11:
                        // passthrough
                case 16:
                        EVE_TEST_EXPECT( res.value( )[i] == i + 1,
                                         "[i] == i+1" );
                        break;
                default:
                        EVE_TEST_EXPECT( res.value( )[i] == C( i + 1 ),
                                         "[i] == C(i+1)" );
                }
        }
        EVE_TEST_PASS( );
}
