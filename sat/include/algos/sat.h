// vim: ft=cpp
#pragma once

#include <optional>
#include <span>
#include <vector>

namespace eve::algos::sat {

/* Defined type for literals. */
using literal_t = size_t;

/* Encode the complement of a literal (1-based).
 *
 * When submit a clause (see Solver), the literal 1 should be submitted as 1,
 * and the complement of literal 1 should be submitted as C(1).
 */
auto C( literal_t c ) -> literal_t;
auto LiteralRawValue( literal_t c ) -> literal_t;
auto LiteralIsC( literal_t c ) -> bool;
auto PrintLiterals( std::span<const literal_t> ) -> void;

class Solver {
      public:
        virtual auto EmitClause( std::span<const literal_t> ) -> void   = 0;
        virtual auto Search( ) -> std::optional<std::vector<literal_t>> = 0;
};

}  // namespace eve::algos::sat
