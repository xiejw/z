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
}  // namespace taocp
