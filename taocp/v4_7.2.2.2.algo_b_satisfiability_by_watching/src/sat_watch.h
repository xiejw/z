// vim: ft=cpp
// forge:v1
#pragma once

#include <vector>

namespace taocp {

/* === --- Defined type for literals --------------------------------------- ===
 *
 * Any caller should use encoded literal when emitting clauses. This helps to
 * wrap both the variable and its complement (see Page 186 of Vol 4B for
 * literal definition). Internal representation is opaque to callers.
 *
 * - To emit variable x, use 'x';
 * - To emit complement of variable x, use 'C(x)'.
 * - Both cases are 1-based literals.
 *
 * Few free functions are provided to decode:
 * - DecodeRawLiteralValue returns the absolute value of the variable x.
 * - IsLiteralComplement returns true if the encoded value is a complement.
 * - PrintClause prints the clause with literals nicely.
 */
using literal_t = size_t;
#define PRI_literal "zu"  // Used for printf.

auto C( literal_t c ) -> literal_t;
auto DecodeRawLiteralValue( literal_t c ) -> literal_t;
auto IsLiteralComplement( literal_t c ) -> bool;
auto PrintClause( size_t size, const literal_t * ) -> void;

/* === --- Defined type for literals --------------------------------------- ===
 */

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
