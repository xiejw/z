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
      m_start( 1 + num_clauses + 1 ),  // Need one more at the end
      m_link( 1 + num_clauses ),
      m_watch( 2 + 2 * num_literals + 1 + num_clauses )
{
        m_cells.push_back( 0 );
}

auto
WatchSolver::ReserveCells( size_t num_cells ) -> void
{
        m_cells.reserve( num_cells );
}

auto
WatchSolver::EmitClause( std::span<const literal_t> lits ) -> void
{
        if ( lits.empty( ) ) {
                panic( "lit empty" );
        }

        auto clause_id = m_num_emitted_clauses + 1;

        if ( m_debug_mode ) {
                if ( clause_id > m_num_clauses ) {
                        panic( "clause num" );
                }

                for ( auto lit : lits ) {
                        auto raw_v = LiteralRawValue( lit );
                        if ( raw_v > m_num_literals ) {
                                panic( "lit" );
                        }
                        if ( raw_v < 1 ) {
                                panic( "lit < 1" );
                        }
                }
        }

        // Put ites
        bool         first_v   = true;
        const size_t watch_bar = GetWatchBar( );
        for ( auto lit : lits ) {
                /* For literal 'l', the value put into the cell is 2*l+C(l). */
                auto raw_v     = LiteralRawValue( lit );
                auto is_c      = LiteralIsC( lit );
                auto literal_v = raw_v * 2 + ( is_c ? 1 : 0 );
                m_cells.push_back( literal_v );

                if ( !first_v ) {
                        continue;
                }

                /* Special block to handle the first literal. */
                first_v = false;

                /* Update the start array. */
                auto pos           = m_cells.size( );
                m_start[clause_id] = pos - 1;

                /* Update the watch list. */
                if ( m_watch[literal_v] == 0 ) {
                        // This branch seems is identical to next.
                        m_watch[literal_v] = watch_bar + clause_id;
                } else {
                        m_watch[watch_bar + clause_id] = m_watch[literal_v];
                        m_watch[literal_v]             = watch_bar + clause_id;
                }
                // link
        }
        m_num_emitted_clauses++;
}

auto
WatchSolver::DebugPrint( ) -> void
{
        std::print( "cells\n" );
        for ( size_t i = 0; i < m_cells.size( ); i++ ) {
                std::print( "{:02} ", i );
        }
        std::print( "\n" );
        for ( size_t i = 0; i < m_cells.size( ); i++ ) {
                std::print( "{:02} ", m_cells[i] );
        }
        std::print( "\n" );

        std::print( "start\n" );
        for ( size_t i = 0; i < m_start.size( ); i++ ) {
                std::print( "{:02} ", i );
        }
        std::print( "\n" );
        for ( size_t i = 0; i < m_start.size( ); i++ ) {
                std::print( "{:02} ", m_start[i] );
        }
        std::print( "\n" );

        std::print( "watch\n" );
        for ( size_t i = 0; i < m_watch.size( ); i++ ) {
                if ( i == GetWatchBar( ) ) {
                        std::print( "|| " );
                }
                std::print( "{:02} ", i );
        }
        std::print( "\n" );
        for ( size_t i = 0; i < m_watch.size( ); i++ ) {
                if ( i == GetWatchBar( ) ) {
                        std::print( "|| " );
                }
                std::print( "{:02} ", m_watch[i] );
        }
        std::print( "\n" );
}
auto
WatchSolver::GetWatchBar( ) const -> size_t
{
        return 2 + 2 * m_num_literals;
}

// TODO How next is updated? seems realy hard why not follow watch?
// TODO record start [0] final one this code is increasting order
}  // namespace eve::algos::sat
