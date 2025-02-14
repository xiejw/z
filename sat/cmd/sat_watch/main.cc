#include <algos/sat_watch.h>

using eve::algos::sat::WatchSolver;

int
main( )
{
        WatchSolver sov{ /*num_literals=*/3, /*num_causes=*/4 };
        return 0;
}
