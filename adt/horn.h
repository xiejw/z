#ifndef ADT_HORN_H_
#define ADT_HORN_H_

#include <stddef.h>  // size_t

#include <adt/types.h>

// === --- Horn Satisfiability --------------------------------------------- ===
//
// #### TLDR
//
// Concludes whether horn formula is satisfiable.
//
// #### Definition
//
// A Horn clause is a clause (a disjunction of literals) with at most one
// positive literal. A Horn formula is a propositional formula formed by
// conjunction of Horn clauses.
//
// #### Definite Horn Formula and The Core
//
// The definite Horn formula must satisfy
//
// ```
// f(1, 1, ..., 1) = 1.
// ```
//
// The _Core_ of the definite Horn formula is set the variables which must be
// true whenever `f` is true.
//
// #### Algorithrm C
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
// #### Indefinite Horn Formula
//
// Exercise 48 provides the steps to test satisfiability of Horn formula in
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
// - Apply _Algorithrm C_ to the new definite Horn formula. The original Horn
//   formula is satisfiable if and only if `lambda` is not in the Core of the
//   new definite Horn formula.
//

// Forward declaration.
struct horn;

extern struct horn *horn_new( int num_propositions );
extern void         horn_free( struct horn * );

// Append a clause with hypotheses and conclusion (-1 if no conclusion)
void horn_add_clause( struct horn *, int conclusion, int num_hypotheses,
                      int *hypotheses );

// Helper macros
#define HORN_ADD_CLAUSE( h, conclusion, num_hy, ... )       \
        horn_add_clause( ( h ), ( conclusion ), ( num_hy ), \
                         (int[]){ __VA_ARGS__ } )
#define HORN_ADD_CLAUSE_WO_CONCLUSION( h, num_hy, ... ) \
        horn_add_clause( ( h ), ( -1 ), ( num_hy ), (int[]){ __VA_ARGS__ } )

#define HORN_ADD_CLAUSE_WO_HYPOTHESES( h, conclusion ) \
        horn_add_clause( ( h ), ( conclusion ), ( 0 ), NULL )

// Return OK if the horn formula is satisfiable; ENOTEXIST otherwise.
error_t horn_search( struct horn * );

// After horn_search returns OK, check whether 0-based proposition (specified
// via prop_id) is in core or not.
//
// Return 1 if in core, 0 otherwise.
//
// NOTE:
// - must be called after horn_search.
// - Core is defined as the propositions which must be true whenever the
//   boolean function is true (see TAOCP, Vol 4a, Page 58), for example, for
//   horn clauses like
//
//       2
//       !0 || 1
//
//   only 2 is in core, 1 is not as both (bar 0, 2) and (1, 2) are solutions
//   only 2 is in the minimum vector to make this boolean function be true.
int horn_is_prop_in_core( struct horn *, int prop_id );

#endif  // ADT_HORN_H_
