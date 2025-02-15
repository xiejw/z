#include <algos/sat_watch.h>

#include <eve/base/error.h>

namespace eve::algos::sat {
namespace {
constexpr size_t mask = 1 << ( sizeof( size_t ) - 1 );
}

auto
C( literal_t c ) -> literal_t
{
        return c | mask;
}

auto
LiteralRawValue( literal_t c ) -> literal_t
{
        return c & ( ~mask );
}

WatchSolver::WatchSolver( size_t num_literals, size_t num_causes )
    : num_literals( num_literals ), num_causes( num_causes )
{
        /* The following vectors are indexed by 1-based causes. */
        this->start.reserve( 1 + num_causes );
        this->link.reserve( 1 + num_causes );
        /* Both literal and cause indices are 1-based . */
        this->watch.reserve( 2 + 2 * num_literals + 1 + num_causes );
}

auto
WatchSolver::EmitCause( std::span<literal_t> lits ) -> void
{
        if ( this->debug_mode ) {
                for ( auto lit : lits ) {
                        if ( LiteralRawValue( lit ) > this->num_literals ) {
                                panic( "lit" );
                        }
                }

                if ( this->num_emitted_causes + 1 > this->num_causes ) {
                        panic( "cause num" );
                }
        }
}
}  // namespace eve::algos::sat
