// vim: ft=cpp
#pragma once

namespace eve::algos::sat {

/* Defined type for literals. */
using literal_t = size_t;

/* Encode the complement of a literal (1-based).
 *
 * When submit a clause (see Solver), the literal 1 should be submitted as 1,
 * and the complement of literal 1 should be submitted as C(1).
 */
auto C( literal_t c ) -> literal_t;

class Solver {
      public:
        virtual auto EmitClause( std::span<const literal_t> ) -> void = 0;
        virtual auto Search( ) -> bool                                = 0;
};

}  // namespace eve::algos::sat
