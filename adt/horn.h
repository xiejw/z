#ifndef ADT_HORN_H_
#define ADT_HORN_H_

#include <stddef.h>  // size_t

#include <adt/types.h>

// === --- Horn Satisfiability --------------------------------------------- ===
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
