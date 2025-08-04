// vim: ft=cpp
//
#pragma once

#include <optional>
#include <span>
#include <vector>

namespace eve::algos::sat {

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
 * - LiteralRawValue returns the absolute value of the x.
 * - LiteralIsC returns true if the encoded value is a complement.
 * - PrintLiterals prints the clause with literals nicely.
 */
using literal_t = size_t;
auto C( literal_t c ) -> literal_t;
auto LiteralRawValue( literal_t c ) -> literal_t;
auto LiteralIsC( literal_t c ) -> bool;
auto PrintLiterals( std::span<const literal_t> ) -> void;

/* === --- SAT Solver Interface  ------------------------------------------- ===
 */
class Solver {
      public:
        virtual auto EmitClause( std::span<const literal_t> ) -> void   = 0;
        virtual auto Search( ) -> std::optional<std::vector<literal_t>> = 0;
};

}  // namespace eve::algos::sat
