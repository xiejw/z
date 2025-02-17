#include <algos/sat_watch.h>

#include <eve/base/error.h>

namespace eve::algos::sat {
namespace {
constexpr size_t mask = 1 << ( sizeof( size_t ) - 1 );

auto
LiteralRawValue( literal_t c ) -> literal_t
{
        return c & ( ~mask );
}

auto
LiteralIsC( literal_t c ) -> bool
{
        return c & mask;
}
}  // namespace

auto
C( literal_t c ) -> literal_t
{
        return c | mask;
}

WatchSolver::WatchSolver( size_t num_literals, size_t num_clauses )
    : m_num_literals( num_literals ),
      m_num_clauses( num_clauses ),
      m_num_emitted_clauses( 0 ),
      m_debug_mode( false ),
      m_start( 1 + num_clauses ),
      m_link( 1 + num_clauses ),
      m_watch( 2 + 2 * num_literals + 1 + num_clauses )
{
}

auto
WatchSolver::EmitClause( std::span<const literal_t> lits ) -> void
{
        if ( lits.empty( ) ) {
                panic( "lit empty" );
        }

        if ( m_debug_mode ) {
                for ( auto lit : lits ) {
                        auto raw_v = LiteralRawValue( lit );
                        if ( raw_v > m_num_literals ) {
                                panic( "lit" );
                        }
                        if ( raw_v < 1 ) {
                                panic( "lit < 1" );
                        }
                }

                if ( m_num_emitted_clauses + 1 > m_num_clauses ) {
                        panic( "clause num" );
                }
        }

        auto clause_id = m_num_emitted_clauses + 1;

        // Put ites
        bool first_v = true;
        for ( auto lit : lits ) {
                auto raw_v  = LiteralRawValue( lit );
                auto is_c   = LiteralIsC( lit );
                auto cell_v = raw_v * 2 + ( is_c ? 1 : 0 );
                m_cells.push_back( cell_v );

                auto pos = m_cells.size( );
                if ( first_v ) {
                        first_v            = false;
                        m_start[clause_id] = pos;
                        // link
                        // watch
                }
        }
        m_num_emitted_clauses++;
}

// TODO record start [0] final one this code is increasting order
}  // namespace eve::algos::sat
