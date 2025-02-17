// vim: ft=cpp
#pragma once

#include <span>
#include <vector>

namespace eve::algos::sat {

using literal_t = size_t;

/* Encode the complement of a literal (1-based). */
auto C( literal_t c ) -> literal_t;

class WatchSolver {
      private:
        size_t m_num_literals;
        size_t m_num_clauses;
        size_t m_num_emitted_clauses;
        bool   m_debug_mode;

        std::vector<size_t> m_cells; /* Index by cell. */
        std::vector<size_t> m_start; /* Index by clause. */
        std::vector<size_t> m_link;  /* Index by clause. */
        std::vector<size_t> m_watch; /* Index by literal and clause. */

      public:
        WatchSolver( size_t num_literals, size_t num_clauses );

      public:
        auto ReserveCells( size_t num_cells ) -> void
        {
                m_cells.reserve( num_cells );
        }
        auto SetDebugMode( bool m ) -> void { m_debug_mode = m; }
        auto EmitClause( std::span<const literal_t> ) -> void;
};
}  // namespace eve::algos::sat
