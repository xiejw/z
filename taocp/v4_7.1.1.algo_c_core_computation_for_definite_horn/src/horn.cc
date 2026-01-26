#include <assert.h>
#include <stdlib.h>  // calloc
#include <string.h>  // memcpy

#include "horn.h"

namespace taocp {

namespace {

/* === --- Data Structures -------------------------------------------------- */

struct horn_clause {
        int id_of_conclusion;  // The proposition on the right of clause. -1 if
                               // absent

        int  num_hypotheses;  // The count of hypotheses.
        int *hypotheses;      // Ids of hypotheses.

        struct horn_clause *prev;  // The previous clause awaiting for the
                                   // same last hypothesis as this one.
};

struct horn_var {
        bool                truth;  // Truth value for this variable
        struct horn_clause *last;   // The last clause in which this
                                    // variable is awaiting to be asserted.
};

struct horn_clause_node {
        struct horn_clause       clause;
        struct horn_clause_node *next;
};
}  // namespace

struct horn {
        int  num_variables;  // Num of proposition.
        bool is_definite;    // whether is definite horn formula.

#ifndef NDEBUG
        bool core_computed;
#endif

        // Variables known to be true but not yet asserted.
        // At most num_variables large.
        int   *stack;
        size_t stack_top;  // Point to next available slot.

        struct horn_clause_node *clauses;  // Linked lists of clauses.
        struct horn_var          vars[1];  // Variables' truth values. Tail arr
};

// === --- Implementation -------------------------------------------------- ===

namespace {

void
horn_core_compute_impl( struct horn *h )
{
        while ( h->stack_top != 0 ) {  // loop until no awaiting prop.
                int prop = h->stack[--h->stack_top];

                struct horn_var *pprop = &h->vars[prop];
                assert( pprop->truth );

                struct horn_clause *c = pprop->last;

                /* Loop over all clauses waiting for same prop */
                while ( c != NULL ) {
                        assert( c->num_hypotheses > 0 );
                        struct horn_clause *c_prev = c->prev;
                        c->num_hypotheses--;

                        for ( int i = c->num_hypotheses - 1; i >= 0; i-- ) {
                                int hypo = c->hypotheses[i];
                                if ( h->vars[hypo].truth ) {
                                        c->num_hypotheses--;
                                        continue;
                                }
                                break;
                        }
                        if ( c->num_hypotheses == 0 ) {
                                struct horn_var *pconc =
                                    &h->vars[c->id_of_conclusion];
                                if ( !pconc->truth ) {
                                        // put id_of_conclusion into the stack.
                                        pconc->truth = 1;
                                        h->stack[h->stack_top++] =
                                            c->id_of_conclusion;
                                }
                        } else {
                                int last_hypo =
                                    c->hypotheses[c->num_hypotheses - 1];
                                struct horn_var *p_last_hypo =
                                    &h->vars[last_hypo];
                                c->prev           = p_last_hypo->last;
                                p_last_hypo->last = c;
                        }

                        /* Prepare for next iteration. */
                        c = c_prev;
                }
        }
}

}  // namespace

// === --- Public APIs ----------------------------------------------------- ===

struct horn *
horn_new( int num_variables )
{
        // '+ 1' prepare for lambda. See horn_is_satisfiable
        size_t n = (size_t)num_variables + 1;

        // C99 style of malloc with tail array.
        struct horn *ptr = (struct horn *)calloc(
            1, sizeof( *ptr ) + n * sizeof( struct horn_var ) );

        ptr->num_variables = num_variables;
        ptr->is_definite   = true;

#ifndef NDEBUG
        ptr->core_computed = false;
#endif

        ptr->stack = (int *)calloc( n, sizeof( int ) );
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
horn_add_clause( struct horn *h, struct horn_conclusion conclusion,
                 int num_hypotheses, int *hypotheses )
{
        assert( !h->core_computed );

        int id_of_conclusion = conclusion.id;

        // Set is_definite.
        if ( id_of_conclusion == -1 ) {
                h->is_definite = 0;
        } else {
                assert( id_of_conclusion < h->num_variables );
        }

        assert( num_hypotheses < h->num_variables );

        // Link into the h->clauses link list.
        struct horn_clause_node *node =
            (struct horn_clause_node *)calloc( 1, sizeof( *node ) );
        node->next = h->clauses;
        h->clauses = node;

        // Fill the clause embeded in the node.
        struct horn_clause *c = &node->clause;
        c->id_of_conclusion   = id_of_conclusion;

        // === --- Fast path if no hypotheses.
        if ( num_hypotheses == 0 ) {
                if ( id_of_conclusion != -1 &&
                     h->vars[id_of_conclusion].truth == 0 ) {
                        /* Flip truth value and put on stack. */
                        h->vars[id_of_conclusion].truth = 1;
                        h->stack[h->stack_top++]        = id_of_conclusion;
                }
                return;
        }

        // === --- Now the slow path.

        // Copy hypotheses.
        c->num_hypotheses = num_hypotheses;
        size_t sz_copy    = sizeof( int ) * (size_t)num_hypotheses;
        c->hypotheses     = (int *)malloc( sz_copy );
        memcpy( c->hypotheses, hypotheses, sz_copy );

        // Link last hypothesis
        int              last_var = hypotheses[num_hypotheses - 1];
        struct horn_var *pvar     = &h->vars[last_var];
        c->prev                   = pvar->last;
        pvar->last                = c;
}

void
horn_core_compute( struct horn *h )
{
        assert( !h->core_computed && ( h->core_computed = true ) );

        // Introdcue lambda.
        int lambda_id = h->num_variables;
        assert( h->vars[lambda_id].truth == 0 );

        // Fill all clauses without conclusions with lambda.
        struct horn_clause_node *node = h->clauses;
        while ( node != NULL ) {
                struct horn_clause_node *next = node->next;
                if ( node->clause.id_of_conclusion == -1 ) {
                        node->clause.id_of_conclusion = lambda_id;
                }
                node = next;
        }

        horn_core_compute_impl( h );
}

// See TAOCP, Vol 4a, Page 543, Exercise 48.
bool
horn_is_satisfiable( struct horn *h )
{
        assert( h->core_computed );
        int lambda_id = h->num_variables;
        return !h->vars[lambda_id].truth;
}

bool
horn_is_var_in_core( struct horn *h, int id_of_var )
{
        assert( h->core_computed );
        assert( id_of_var < h->num_variables );
        return h->vars[id_of_var].truth;
}
}  // namespace taocp
