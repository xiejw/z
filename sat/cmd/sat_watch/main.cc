#include <array>
#include <print>

#include <algos/sat_watch.h>

using eve::algos::sat::C;
using eve::algos::sat::WatchSolver;

int
main( )
{
        WatchSolver sov{ /*num_literals=*/3, /*num_causes=*/4 };
        sov.SetDebugMode( true );

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
        sov.DebugPrint( );

        if ( sov.Search( ) ) {
                std::print( "good\n" );
        } else {
                std::print( "bad\n" );
        }

        return 0;
}
