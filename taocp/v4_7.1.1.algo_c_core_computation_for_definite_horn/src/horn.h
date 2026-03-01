// vim: ft=cpp
//
// forge:v1
//
// === --- Horn Satisfiability Vol 4A 7.1.1 Algorithm C Page 59 ----------- ===
//
// === --- TLDR
//
// - Compures the core of definite horn function.
// - Concludes whether horn function is satisfiable.
//
// Key idea:
//
//     Linearly flow information from conclusions (right hand side, the Core) to
//     propositional variables (left hand side) and see whether there is
//     solutions.
//
// === --- Definition of Horn Clause
//
// A Horn clause is a clause (a disjunction of literals) with at most one
// positive literal. A Horn function is a propositional formula formed by
// conjunction of Horn clauses.
//
// === --- Definite Horn Function and The Core
//
// The definite Horn function must satisfy
//
// ```
// f(1, 1, ..., 1) = 1.
// ```
//
// The _Core_ of the definite Horn function is set the variables which must be
// true whenever `f` is true.
//
// === --- Algorithm C (Vol 4A, Page 59)
//
// The algorithm is as follows:
//
// - Put all positive literal in the single variable clause into Core.  They
//   must be true.
//
// - Keep deducing the proposition of non-positive literal, whenever their
//   values are known to be true, i.e., in Core. Once all non-positive literals
//   in a clause are deduced, its positive literal, if present, must be in
//   Core.
//
// === --- Indefinite Horn Function
//
// Exercise 48 provides the steps to test satisfiability of Horn function in
// general. The idea is quite simple:
//
// - Introduce a new variable `lambda`, and convert all indefinite causes to
//   definite cause. For example, the following indefinite Horn clause
//
//   ```
//   !a || !b
//   ```
//
//   will be converted as
//
//   ```
//   !a || !b || lambda
//   ```
//
// - Apply _Algorithm C_ to the new definite Horn function. The original Horn
//   function is satisfiable if and only if `lambda` is not in the Core of the
//   new definite Horn function.
//
#pragma once

#include <stddef.h>  // size_t

namespace taocp {

// Forward declaration.
struct horn;

// A wrapper to make conclusion strong type.
struct horn_conclusion {
        int id;  // -1 if no conclusion.
};

inline struct horn_conclusion
horn_no_conclusion( )
{
        return horn_conclusion{ -1 };
}

struct horn *horn_new( int num_variables );
void         horn_free( struct horn * );

// Append a clause with hypotheses and conclusion (-1 if no conclusion)
void horn_add_clause( struct horn *, struct horn_conclusion conclusion,
                      int num_hypotheses, int *id_of_hypotheses );

// Compute the core of horn clauses.
//
// NOTE: cannot be called twice on the struct horn as it mutates the data
// structure.
//
// See TAOCP, vol 4a, Page 59, Algorithm C.
void horn_core_compute( struct horn *h );

// Return true if the horn function is satisfiable; false otherwise.
//
// NOTE: Must be called after horn_core_compute.
//
// - See TAOCP, Vol 4a, Page 543, Exercise 48.
[[nodiscard]] bool horn_is_satisfiable( struct horn * );

// After horn_is_satisfiable returns, check whether 0-based variable (specified
// via id_of_var) is in core or not.
//
// Return true if in core, false otherwise.
//
// NOTE:
// - NOTE: Must be called after horn_core_compute.
// - Core is defined as the variables which must be true whenever the
//   boolean function is true (see TAOCP, Vol 4a, Page 58), for example, for
//   horn clauses like
//
//       2
//       !0 || 1
//
//   only 2 is in core, 1 is not as both (bar 0, 2) and (1, 2) are solutions
//   only 2 is in the minimum vector to make this boolean function be true.
bool horn_is_var_in_core( struct horn *h, int id_of_var );

///////// TODO

// Helper macros
#define HORN_ADD_CLAUSE_HELPER( h, conclusion, num_hy, ... )        \
        do {                                                        \
                int hypotheses[] = { __VA_ARGS__ };                 \
                horn_add_clause( ( h ), ( conclusion ), ( num_hy ), \
                                 hypotheses );                      \
        } while ( 0 )

#define HORN_ADD_CLAUSE_WO_HYPOTHESES( h, conclusion ) \
        horn_add_clause( ( h ), ( conclusion ), ( 0 ), NULL )

}  // namespace taocp
