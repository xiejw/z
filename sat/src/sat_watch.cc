#include "sat.h"

#include <print>

#include <zion/zion.h>

#define DEBUG_MODE 0

namespace eos::sat {

WatchSolver::WatchSolver( size_t num_literals, size_t num_clauses,
                          size_t num_reserved_cells )
    : m_num_literals( num_literals ),
      m_num_clauses( num_clauses ),
      m_num_emitted_clauses( 0 ),
      m_start( 1 + num_clauses ),
      m_watch( 2 + 2 * num_literals ),
      m_link( 1 + num_clauses )
{
        m_cells.reserve( 1 + num_reserved_cells );
        m_cells.push_back( 0 );
}

WatchSolver::WatchSolver( size_t num_literals, size_t num_clauses )
    : WatchSolver( num_literals, num_clauses, 5 * num_clauses )
{
}

auto
WatchSolver::reserve_cells( size_t num_cells ) -> void
{
        m_cells.reserve( 1 + num_cells );
}

auto
WatchSolver::emit_clause( std::span<const literal_t> lits ) -> void
{
        /* === --- Few quick sanity checks. ----------------------------- === */
        if ( lits.empty( ) ) PANIC( "emitted clause cannot be empty." );
        if ( m_num_emitted_clauses >= m_num_clauses )
                PANIC( "emitted clause is full. Cannot submit one more." );

        this->debug_check( lits );

        /* Clause id is 1-based, and decreasing order. */
        auto clause_id = m_num_clauses - m_num_emitted_clauses;

        /* Put each literal into the internal data structures. */
        bool first_v = true;
        for ( auto lit : lits ) {
                /* For literal 'l', the value put into the cell is 2*l+C(l).
                 */
                auto raw_v     = decode_literal_raw_value( lit );
                auto is_c      = is_literal_C( lit );
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
                /* Once all clauses are emitted, the start[0] - 1 should
                 * point to the final cell. */
                m_start[0] = m_cells.size( );
        }
}

auto
WatchSolver::search( ) -> std::optional<std::vector<literal_t>>
{
        if ( m_num_emitted_clauses != m_num_clauses ) {
                PANIC( "emitted clauses are not enough. expected {}, got {}",
                       (size_t)m_num_clauses, (size_t)m_num_emitted_clauses );
        }
        /* === --- This algorithm is Vol 4b, Page 215. ------------------ === */

        /* B1 Init */
        size_t d = 1;
        size_t n = m_num_literals;

        std::vector<size_t> m( n + 1 );

        /* B2 Rejoice or choose */
        while ( d <= n ) {
                m[d] = m_watch[2 * d] == 0 || m_watch[2 * d + 1] != 0;
                // size_t l      = 2 * d + m[d];
                size_t comp_l = 2 * d + ( m[d] ^ 1 );

        B3:
                /* B3 Remove C(l) if possible. Page 573. Ex 124 */
                size_t j = m_watch[comp_l];

                if ( DEBUG_MODE )
                        DEBUG(
                            "work at B3 at level d {} to remove compliment of "
                            "literal with starting clause {} (comp l = {})",
                            (size_t)d, j, comp_l );

                while ( j != 0 ) {
                        if ( DEBUG_MODE ) {
                                DEBUG(
                                    "--> sub level B3 to work on clause j = "
                                    "{}",
                                    (size_t)j );
                                dump_debug_info( );
                        }
                        /* A literal other than comp_l should be watched in
                         * clause j. */

                        size_t begin  = m_start[j];
                        size_t end    = m_start[j - 1];
                        size_t next_j = m_link[j];

                        if ( DEBUG_MODE ) {
                                if ( next_j > m_num_clauses ) {
                                        dump_debug_info( );
                                        PANIC( "wrong next j" );
                                }
                        }

                        size_t k = begin + 1;
                        for ( ; k < end; k++ ) {
                                if ( DEBUG_MODE )
                                        DEBUG(
                                            "--> --> sub level B3 clause j = "
                                            "{} work on cell {} (begin {}, "
                                            "end {})",
                                            (size_t)j, (size_t)k, begin, end );

                                size_t new_l = m_cells[k];

                                /* If new_l isn't false. Swap it to
                                 * beginning. */
                                if ( ( new_l >> 1 ) > d ||
                                     ( ( new_l + m[new_l >> 1] ) % 2 ) == 0 ) {
                                        m_cells[begin] = new_l;
                                        m_cells[k]     = comp_l;
                                        m_link[j]      = m_watch[new_l];
                                        m_watch[new_l] = j;
                                        j              = next_j;
                                        break; /* This clause is done. */
                                }
                        }

                        if ( k == end ) {
                                /* Cannot stop watching on comp_l. */
                                m_watch[comp_l] = j;
                                if ( DEBUG_MODE )
                                        DEBUG(
                                            "cannot stop watching on "
                                            "compliment of literal, go to B5 "
                                            "at level d {}",
                                            (size_t)d );
                                goto B5;
                        }
                }

                /* B4 Advance */
                m_watch[comp_l] = 0;
                d++;
                if ( DEBUG_MODE )
                        DEBUG( "advance to with B2 at level d {}", (size_t)d );
                continue; /* Return to B2 */

        B5:
                /* B5 Try again */
                if ( m[d] < 2 ) {
                        size_t new_md = 3 - m[d];
                        m[d]          = new_md;
                        // l             = 2 * d + ( new_md & 1 );
                        comp_l = 2 * d + ( ( new_md & 1 ) ^ 1 );

                        if ( DEBUG_MODE )
                                DEBUG(
                                    "try again with B3 at level d {} and m[d] "
                                    "= {}",
                                    (size_t)d, (size_t)new_md );
                        goto B3;
                } else {
                        /* B6 Backtrack */
                        if ( d == 1 ) return std::nullopt; /* Failed */

                        /* Otherwise */
                        d--;

                        if ( DEBUG_MODE )
                                DEBUG( "backtrack with B5 at level d {}",
                                       (size_t)d );
                        goto B5;
                }
        }

        std::vector<literal_t> result( m_num_literals );

        for ( size_t i = 1; i <= m_num_literals; i++ ) {
                if ( m[i] & 1 ) {
                        result[i - 1] = C( i );
                } else {
                        result[i - 1] = i;
                }
        }

        return result; /* Exit happily */
}

auto
WatchSolver::dump_debug_info( ) -> void
{
        /* === --- Internal --------------------------------------------- === */
        std::print( "internal\n" );
        std::print( "  num_literals {:3}\n", m_num_literals );
        std::print( "  num_clauses  {:3}\n", m_num_clauses );

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
WatchSolver::debug_check( std::span<const literal_t> lits ) const -> void
{
        for ( auto lit : lits ) {
                auto raw_v = decode_literal_raw_value( lit );
                if ( raw_v > m_num_literals ) {
                        PANIC( "lit" );
                }
                if ( raw_v < 1 ) {
                        PANIC( "lit < 1" );
                }
        }
}

}  // namespace eos::sat
