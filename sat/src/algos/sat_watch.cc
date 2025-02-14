#include <algos/sat_watch.h>

namespace eve::algos::sat {
namespace {
constexpr size_t mask = 1 << ( sizeof( size_t ) - 1 );
}

auto
C( size_t c ) -> size_t
{
        return c | mask;
}

WatchSolver::WatchSolver( size_t num_literals, size_t num_causes )
{
        /* Literal index is 1-based. */
        this->cells.reserve( 2 + 2 * num_literals );
        /* The following vectors are indexed by 1-based causes. */
        this->start.reserve( 1 + num_causes );
        this->link.reserve( 1 + num_causes );
        /* Both literal and cause indices are 1-based . */
        this->watch.reserve( 2 + 2 * num_literals + 1 + num_causes );
}
}  // namespace eve::algos::sat
