// vim: ft=cpp
// forge:v1
#pragma once

#include <initializer_list>
#include <vector>
#include <optional>

#include "sat_literal.h"

namespace taocp {

/* === --- Defined type for literals --------------------------------------- ===
 *
 * Algorithm B: Satisfiability by Watching. Vol 4B Page 215.
 */

class WatchSolver {
      private:
        size_t m_num_literals;
        size_t m_num_clauses;
        size_t m_num_emitted_clauses;

        std::vector<size_t> m_cells; /* Index by cell. */

        /*
         * The start position for each clause in cells.  The clause is
         * reversely ordered. And each literal is reversely ordered in the
         * clause. The watched literal is at the first position of the clause.

         *
         * Index by clause. 1-based.
         */
        std::vector<size_t> m_start;

        /*
         * The start pointer for the clause watching the literal. The watched
         * literal is at the first position of the clause.
         *
         * Index by literal. 1-based.
         */
        std::vector<literal_t> m_watch;

        /*
         * LINK(j) is the pointer to the next clause with the same watched
         * literal or 0 if last such clause.
         *
         * Index by clause. 1-based.
         */
        std::vector<size_t> m_link;

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
        void EmitClause( size_t size, const literal_t * );
        void EmitClause( std::initializer_list<literal_t> );
        auto SearchOneSolution( ) -> std::optional<std::vector<literal_t>>;

      public:
        /* === --- A set of debug tooling. ----------------------------- === */

        /* Print the internal states of the solver. Orthogonal to debug mode. */
        auto dump_debug_info( ) -> void;

      private:
        /// Validate all literals are in right value domain.
        auto DebugCheck( size_t size, const literal_t * ) const -> void;
};
}  // namespace taocp
