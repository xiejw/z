#ifndef OPUS_HORN_H_
#define OPUS_HORN_H_

#include <stddef.h>  // size_t

// =============================================================================
// horn satisfiability
// =============================================================================

// -----------------------------------------------------------------------------
// public apis
//

struct horn;

extern struct horn *hornNew( int num_propositions );
extern void         hornFree( struct horn * );

// append a clause with hypotheses and conclusion (-1 if no conclusion)
extern void hornAddClause( struct horn *, int conclusion, int num_hypotheses,
                           int *hypotheses );

// helper macros
#define HORN_NO_CONC -1

#define HORN_ADD_C0( h, conclude ) hornAddClause( ( h ), ( conclude ), 0, NULL )
#define HORN_ADD_C1( h, conclude, x ) \
    HORN_ADD_C( ( h ), ( conclude ), 1, ( x ) )
#define HORN_ADD_C2( h, conclude, x, y ) \
    HORN_ADD_C( ( h ), ( conclude ), 2, ( x ), ( y ) )

// general macro
#define HORN_ADD_C( h, conclude, num_hy, ... ) \
    hornAddClause( ( h ), ( conclude ), ( num_hy ), (int[]){ __VA_ARGS__ } )

// return 1 if the horn formula is satisfiable; 0 otherwise.
extern int hornSolve( struct horn * );

// -----------------------------------------------------------------------------
// data structure: horn clause and formula
//

struct horn_clause {
    int                 conclusion;  // the proposition on the right of clause.
    int                 num_hypotheses;  // the count of hypotheses.
    int                *hypotheses;      // ids of hypotheses.
    struct horn_clause *prev;            // the previous clause awaiting for the
                                         // same hypotheses as this one.
};

struct horn_prop {
    short               truth;  // truth value.
    struct horn_clause *last;   // the last clause in which this
                                // proposition is awaiting to be asserted.
};

struct horn_clause_node {
    struct horn_clause       clause;
    struct horn_clause_node *next;
};

struct horn {
    int    num_propositions;  // num of proposition.
    int    is_definite;       // is definite horn formula.
    int   *stack;      // propositions known to be true but not yet asserted.
    size_t stack_top;  // point to next avaiable slot.
    struct horn_clause_node *clauses;  // linked lists of clauses.
    struct horn_prop         props[];  //  allocation of propositions.
};

// -----------------------------------------------------------------------------
// internal api
//
// find all props in core.
extern void hornCore( struct horn * );

#endif  // OPUS_HORN_H_
