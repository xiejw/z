// vim: ft=cpp
//
#pragma once

#include <optional>
#include <span>
#include <vector>

namespace eos::sat {

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

/* === --- SAT Solver Interface  ------------------------------------------- ===
 */
class Solver {
      public:
        virtual auto emit_clause( size_t size, const literal_t * ) -> void = 0;
        virtual auto search( ) -> std::optional<std::vector<literal_t>>    = 0;
};

}  // namespace eos::sat
