// vim: ft=cpp
#pragma once

#include <vector>

namespace taocp {

/* === --- Defined type for literals --------------------------------------- ===
 *
 * Any caller should use encoded literal when emitting clauses. This helps to
 * wrap both the literal and its complement. Internal representation is opaque
 * to callers.
 *
 * - To emit literal x, use 'x';
 * - To emit complement of literal x, use 'C(x)'.
 * - Both cases are 1-based literals.
 *
 * Few free functions are provided to decode:
 * - decode_literal_raw_value returns the absolute value of the x.
 * - is_literal_C returns true if the encoded value is a complement.
 * - print_clause_literals prints the clause with literals nicely.
 */
using literal_t = size_t;
auto C( literal_t c ) -> literal_t;
auto decode_literal_raw_value( literal_t c ) -> literal_t;
auto is_literal_C( literal_t c ) -> bool;
auto print_clause_literals( size_t size, const literal_t * ) -> void;

class WatchSolver {
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

        auto reserve_cells( size_t num_cells ) -> void;

      public:
        /* === --- Conform Base Class ----------------------------------- === */
        void emit_clause( size_t size, const literal_t * );
        auto search( ) -> std::optional<std::vector<literal_t>>;

      public:
        /* === --- A set of debug tooling. ----------------------------- === */

        /* Print the internal states of the solver. Orthogonal to debug mode. */
        auto dump_debug_info( ) -> void;

      private:
        auto debug_check( size_t size, const literal_t * ) const -> void;
};
}  // namespace taocp
