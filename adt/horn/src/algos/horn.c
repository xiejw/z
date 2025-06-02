#include <algos/horn.h>

#include <assert.h>
#include <stdlib.h>  // calloc
#include <string.h>  // memcpy
                     //
// -----------------------------------------------------------------------------
// public api
// -----------------------------------------------------------------------------

struct horn *
hornNew( int num_propositions )
{
    // '+ 1' prepare for lambda. See hornCore
    size_t n = (size_t)num_propositions + 1;

    struct horn *ptr =
        calloc( 1, sizeof( *ptr ) + n * sizeof( struct horn_prop ) );
    ptr->num_propositions = num_propositions;
    ptr->is_definite      = 1;
    ptr->stack            = calloc( n, sizeof( int ) );
    return ptr;
}

void
hornFree( struct horn *ptr )
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

// see TAOCP, vol 4a, pg 59, Algorithrm C.
void
hornAddClause( struct horn *h, int conclusion, int num_hypotheses,
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

    if ( num_hypotheses == 0 ) {
        c->conclusion = conclusion;
        if ( conclusion != -1 && h->props[conclusion].truth == 0 ) {
            h->props[conclusion].truth = 1;
            h->stack[h->stack_top++]   = conclusion;
        }
        return;
    }

    c->conclusion = conclusion;

    // copy hypotheses.
    c->num_hypotheses = num_hypotheses;
    size_t sz_copy    = sizeof( int ) * (size_t)num_hypotheses;
    c->hypotheses     = malloc( sz_copy );
    memcpy( c->hypotheses, hypotheses, sz_copy );

    // link last hypothesis
    int               last_prop = hypotheses[num_hypotheses - 1];
    struct horn_prop *ptr_prop  = &h->props[last_prop];
    c->prev                     = ptr_prop->last;
    ptr_prop->last              = c;
}

// see TAOCP, vol 4a, pg ??, Exercise 48.
int
hornSolve( struct horn *h )
{
    if ( h->is_definite ) {
        return 1;
    }

    int                      lambda_id = h->num_propositions;
    struct horn_clause_node *node      = h->clauses;
    while ( node != NULL ) {
        struct horn_clause_node *next = node->next;
        if ( node->clause.conclusion == -1 ) {
            node->clause.conclusion = lambda_id;
        }
        node = next;
    }

    hornCore( h );

    return h->props[lambda_id].truth == 0;
}

// -----------------------------------------------------------------------------
// internal api
// -----------------------------------------------------------------------------

// see TAOCP, vol 4a, pg 59, Algorithrm C.
void
hornCore( struct horn *h )
{
    while ( h->stack_top != 0 ) {  // loop until no awaiting prop.
        int prop = h->stack[--h->stack_top];

        struct horn_prop *pprop = &h->props[prop];
        assert( pprop->truth );

        struct horn_clause *c = pprop->last;

        while ( c != NULL ) {  // loop over all clauses waiting for same p
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
                struct horn_prop *pconc = &h->props[c->conclusion];
                if ( !pconc->truth ) {
                    // put conclusion into the stack.
                    pconc->truth             = 1;
                    h->stack[h->stack_top++] = c->conclusion;
                }
            } else {
                int last_hypo = c->hypotheses[c->num_hypotheses - 1];
                struct horn_prop *p_last_hypo = &h->props[last_hypo];
                c->prev                       = p_last_hypo->last;
                p_last_hypo->last             = c;
            }

            // prepare for next iteration.
            c = c_prev;
        }
    }
}
