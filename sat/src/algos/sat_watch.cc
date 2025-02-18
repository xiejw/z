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
      m_watch( 2 + 2 * num_literals ),
      m_link( 1 + num_clauses )
{
        m_cells.reserve( 1 + 3 * num_clauses );  // Make a guess.
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

        /* Clause id is decreasing order. */
        auto clause_id = m_num_clauses - m_num_emitted_clauses;

        if ( m_debug_mode ) this->DebugCheck( m_num_emitted_clauses + 1, lits );

        // Put ites
        bool first_v = true;
        for ( auto lit : lits ) {
                /* For literal 'l', the value put into the cell is 2*l+C(l). */
                auto raw_v     = LiteralRawValue( lit );
                auto is_c      = LiteralIsC( lit );
                auto literal_v = raw_v * 2 + ( is_c ? 1 : 0 );
                m_cells.push_back( literal_v );

                if ( !first_v ) continue;

                /* === --- Special block to handle the first literal. ------ ===
                 *
                 * Start, Watch, Link should be recorded correctly.
                 */
                first_v = false;

                /* Update the start array. */
                auto pos           = m_cells.size( );
                m_start[clause_id] = pos - 1;

                /* Update the watch list. */
                if ( m_watch[literal_v] == 0 ) {
                        // This branch seems is identical to next.
                        m_watch[literal_v] = clause_id;
                } else {
                        m_link[clause_id]  = m_watch[literal_v];
                        m_watch[literal_v] = clause_id;
                }
        }

        m_num_emitted_clauses++;

        if ( m_num_emitted_clauses == m_num_clauses ) {
                /* Once all clauses are emitted, the start[0] - 1 should point
                 * to the final cell. */
                m_start[0] = m_cells.size( );
        }
}

auto
WatchSolver::DebugPrint( ) -> void
{
        /* === --- Cells ------------------------------------------------ === */
        std::print( "cells\n" );
        for ( size_t i = 0; i < m_cells.size( ); i++ ) {
                std::print( "{:02} ", i );
        }
        std::print( "\n" );
        for ( size_t i = 0; i < m_cells.size( ); i++ ) {
                std::print( "{:02} ", m_cells[i] );
        }
        std::print( "\n" );

        /* === --- Start ------------------------------------------------ === */
        std::print( "start\n" );
        for ( size_t i = 0; i < m_start.size( ); i++ ) {
                std::print( "{:02} ", i );
        }
        std::print( "\n" );
        for ( size_t i = 0; i < m_start.size( ); i++ ) {
                std::print( "{:02} ", m_start[i] );
        }
        std::print( "\n" );

        /* === --- Watch ------------------------------------------------ === */
        std::print( "watch\n" );
        for ( size_t i = 0; i < m_watch.size( ); i++ ) {
                std::print( "{:02} ", i );
        }
        std::print( "\n" );
        for ( size_t i = 0; i < m_watch.size( ); i++ ) {
                std::print( "{:02} ", m_watch[i] );
        }
        std::print( "\n" );

        /* === --- Link ------------------------------------------------- === */
        std::print( "link\n" );
        for ( size_t i = 0; i < m_link.size( ); i++ ) {
                std::print( "{:02} ", i );
        }
        std::print( "\n" );
        for ( size_t i = 0; i < m_link.size( ); i++ ) {
                std::print( "{:02} ", m_link[i] );
        }
        std::print( "\n" );
}
auto
WatchSolver::DebugCheck( size_t                     num_emitted_clauses,
                         std::span<const literal_t> lits ) const -> void
{
        if ( num_emitted_clauses > m_num_clauses ) {
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

}  // namespace eve::algos::sat
