#include <algos/sat_watch.h>
#include <array>

using eve::algos::sat::C;
using eve::algos::sat::WatchSolver;

int
main( )
{
        WatchSolver           sov{ /*num_literals=*/3, /*num_causes=*/4 };
        std::array<size_t, 2> a{ 1, C( 1 ) };
        sov.EmitClause( a );
        return 0;
}
