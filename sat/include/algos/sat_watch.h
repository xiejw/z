// vim: ft=cpp
#pragma once

#include <span>
#include <vector>

#include <algos/sat.h>

namespace eve::algos::sat {

class WatchSolver : Solver {
      private:
        size_t m_num_literals;
        size_t m_num_clauses;
        size_t m_num_emitted_clauses;

        std::vector<size_t> m_cells; /* Index by cell. */
        std::vector<size_t> m_start; /* Index by clause. */
        std::vector<size_t> m_watch; /* Index by literal. */
        std::vector<size_t> m_link;  /* Index by clause. */

      public:
        /* === --- Constructors ----------------------------------------- === */

        /* For all constructors, num_literals and num_clauses are fixed and
         * cannot be changed anymore. num_reserved_cells should be the best
         * guess for the application. It requires one more space for the
         * placeholder. */
        WatchSolver( size_t num_literals, size_t num_clauses,
                     size_t num_reserved_cells );
        WatchSolver( size_t num_literals, size_t num_clauses );

        auto ReserveCells( size_t num_cells ) -> void;

      public:
        /* === --- Conform Base Class ----------------------------------- === */
        auto EmitClause( std::span<const literal_t> ) -> void override;
        auto Search( ) -> std::optional<std::vector<literal_t>> override;

      public:
        /* === --- A set of debug tooling. ----------------------------- === */

        /* Print the internal states of the solver. Orthogonal to debug mode. */
        auto DebugPrint( ) -> void;

      private:
        auto DebugCheck( std::span<const literal_t> ) const -> void;
};
}  // namespace eve::algos::sat
