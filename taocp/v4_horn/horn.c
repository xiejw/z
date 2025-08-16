#include <assert.h>
#include <stdlib.h>  // calloc
#include <string.h>  // memcpy

#include "horn.h"

/* === --- Data Structures -------------------------------------------------- */

struct horn_clause {
        int conclusion;  // The proposition on the right of clause. -1 if absent
        int num_hypotheses;              // The count of hypotheses.
        int                *hypotheses;  // Ids of hypotheses.
        struct horn_clause *prev;        // The previous clause awaiting for the
                                         // same last hypothesis as this one.
};

struct horn_prop {
        short               truth;  // Truth value for this proposition
        struct horn_clause *last;   // The last clause in which this
                                    // proposition is awaiting to be asserted.
};

struct horn_clause_node {
        struct horn_clause       clause;
        struct horn_clause_node *next;
};

struct horn {
        int  num_propositions;  // Num of proposition.
        int  is_definite;       // whether is definite horn formula.
        int *stack;  // Propositions known to be true but not yet asserted.
                     // At most num_propositions large.
        size_t                   stack_top;  // Point to next available slot.
        struct horn_clause_node *clauses;    // Linked lists of clauses.
        struct horn_prop         props[];    //  Propositions' truth values.
};

/* === --- Private helper utils --------------------------------------------- */

// Compute the core of horn clauses.
//
// See TAOCP, vol 4a, Page 59, Algorithm C.
static void
horn_core_compute( struct horn *h )
{
        while ( h->stack_top != 0 ) {  // loop until no awaiting prop.
                int prop = h->stack[--h->stack_top];

                struct horn_prop *pprop = &h->props[prop];
                assert( pprop->truth );

                struct horn_clause *c = pprop->last;

                /* Loop over all clauses waiting for same prop */
                while ( c != NULL ) {
                        assert( c->num_hypotheses > 0 );
                        struct horn_clause *c_prev = c->prev;
                        c->num_hypotheses--;

                        for ( int i = c->num_hypotheses - 1; i >= 0; i-- ) {
                                int hypo = c->hypotheses[i];
                                if ( h->props[hypo].truth ) {
                                        c->num_hypotheses--;
                                        continue;
                                }
                                break;
                        }
                        if ( c->num_hypotheses == 0 ) {
                                struct horn_prop *pconc =
                                    &h->props[c->conclusion];
                                if ( !pconc->truth ) {
                                        // put conclusion into the stack.
                                        pconc->truth = 1;
                                        h->stack[h->stack_top++] =
                                            c->conclusion;
                                }
                        } else {
                                int last_hypo =
                                    c->hypotheses[c->num_hypotheses - 1];
                                struct horn_prop *p_last_hypo =
                                    &h->props[last_hypo];
                                c->prev           = p_last_hypo->last;
                                p_last_hypo->last = c;
                        }

                        /* Prepare for next iteration. */
                        c = c_prev;
                }
        }
}

// === --- Implementation -------------------------------------------------- ===

struct horn *
horn_new( int num_propositions )
{
        // '+ 1' prepare for lambda. See horn_search
        size_t n = (size_t)num_propositions + 1;

        struct horn *ptr =
            calloc( 1, sizeof( *ptr ) + n * sizeof( struct horn_prop ) );
        ptr->num_propositions = num_propositions;
        ptr->is_definite      = 1;
        ptr->stack            = calloc( n, sizeof( int ) );
        return ptr;
}

void
horn_free( struct horn *ptr )
{
        if ( ptr == NULL ) return;

        struct horn_clause_node *node = ptr->clauses;
        while ( node != NULL ) {
                struct horn_clause_node *next = node->next;
                free( node->clause.hypotheses );
                free( node );
                node = next;
        }
        free( ptr->stack );
        free( ptr );
}

// See TAOCP, Vol 4a, Page 59, Algorithm C.
void
horn_add_clause( struct horn *h, int conclusion, int num_hypotheses,
                 int *hypotheses )
{
        if ( conclusion == -1 ) {
                h->is_definite = 0;
        } else {
                assert( conclusion < h->num_propositions );
        }
        assert( num_hypotheses < h->num_propositions );

        struct horn_clause_node *node = calloc( 1, sizeof( *node ) );
        node->next                    = h->clauses;
        h->clauses                    = node;

        struct horn_clause *c = &node->clause;
        c->conclusion         = conclusion;

        if ( num_hypotheses == 0 ) {
                if ( conclusion != -1 && h->props[conclusion].truth == 0 ) {
                        /* Flip truth value and put on stack. */
                        h->props[conclusion].truth = 1;
                        h->stack[h->stack_top++]   = conclusion;
                }
                return;
        }

        // Copy hypotheses.
        c->num_hypotheses = num_hypotheses;
        size_t sz_copy    = sizeof( int ) * (size_t)num_hypotheses;
        c->hypotheses     = malloc( sz_copy );
        memcpy( c->hypotheses, hypotheses, sz_copy );

        // Link last hypothesis
        int               last_prop = hypotheses[num_hypotheses - 1];
        struct horn_prop *ptr_prop  = &h->props[last_prop];
        c->prev                     = ptr_prop->last;
        ptr_prop->last              = c;
}

// See TAOCP, Vol 4a, Page 543, Exercise 48.
int
horn_search( struct horn *h )
{
        int lambda_id = h->num_propositions;
        assert( h->props[lambda_id].truth == 0 );

        struct horn_clause_node *node = h->clauses;
        while ( node != NULL ) {
                struct horn_clause_node *next = node->next;
                if ( node->clause.conclusion == -1 ) {
                        node->clause.conclusion = lambda_id;
                }
                node = next;
        }

        horn_core_compute( h );

        return h->props[lambda_id].truth == 1 ? ENOTEXIST : OK;
}

int
horn_is_prop_in_core( struct horn *h, int prop_id )
{
        return h->props[prop_id].truth;
}
