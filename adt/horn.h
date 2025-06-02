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
// A helper macro
#define HORN_ADD_CLAUSE( h, conclude, num_hy, ... )       \
        horn_add_clause( ( h ), ( conclude ), ( num_hy ), \
                         (int[]){ __VA_ARGS__ } )
#define HORN_ADD_CLAUSE_WO_CONCLUSION( h, num_hy, ... ) \
        horn_add_clause( ( h ), ( -1 ), ( num_hy ), (int[]){ __VA_ARGS__ } )

// Return OK if the horn formula is satisfiable; ENOTEXIST otherwise.
error_t horn_search( struct horn * );

#endif  // ADT_HORN_H_
