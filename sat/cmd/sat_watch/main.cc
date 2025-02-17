#include <algos/sat_watch.h>
#include <array>

using eve::algos::sat::C;
using eve::algos::sat::WatchSolver;

int
main( )
{
        WatchSolver sov{ /*num_literals=*/3, /*num_causes=*/4 };
        sov.EmitClause( {
            { 1, C( 1 ) }
        } );
        sov.EmitClause( {
            { 1, C( 2 ) }
        } );
        sov.DebugPrint( );

        return 0;
}
