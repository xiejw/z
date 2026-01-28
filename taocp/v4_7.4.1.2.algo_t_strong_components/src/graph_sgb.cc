// forge:v1
//
#include "graph_sgb.h"

#include "assert.h"

namespace taocp {
namespace {
void
Run( SGB *g )
{
        // Only used for assignment and comparision.
        SGBNode *const SENT  = &g->vertices.data( )[g->vertices.size( )];
        SGBNode *const START = &g->vertices.data( )[0];
        size_t         n_g   = g->vertices.size( );

        size_t           p;
        SGBNode         *SINK;
        SGBNode         *ROOT;
        SGBNode         *w;
        SGBNode         *v;
        SGBNode         *u;
        SGBNode::ArcIter a;

        goto T1;

T1:  // Initialize

        for ( auto &v : g->vertices ) {
                v.parent = NULL;
                v.arc    = v.arcs.begin( );
        }
        w    = SENT;
        p    = 0;
        SINK = SENT;

        // T2:  // Done?
        if ( w == START ) return;

        do {
                w--;
        } while ( w->parent != NULL );

        v         = w;
        v->parent = SENT;
        ROOT      = v;

T3:  // Begin to explore from v;
        a = v->arcs.begin( );
        p++;
        v->rep  = p;
        v->link = SENT;

T4:  // Done with v
        if ( a == v->arcs.end( ) ) goto T7;

        // T5: // Visit next arc, v -> u
        u = *a;
        a++;

        // T6: // if u is new move to it
        if ( u->parent == NULL ) {
                u->parent = v;
                v->arc    = a;  // Store the state. T3 will overwrite a.
                v         = u;
                goto T3;
        }

        // TODO; WHy???
        if ( u == ROOT && p == n_g ) {  // Last component
                while ( v != ROOT ) {
                        // Link into unsetteled nodes.
                        v->link = SINK;
                        SINK    = v;
                        v       = v->parent;
                }
                u = SENT;
                goto T8;
        }

        if ( u->rep < v->rep ) {
                v->rep  = u->rep;
                v->link = NULL;
        }

        goto T4;

T7:  // Finish with v;
        u = v->parent;
        if ( v->link == SENT ) goto T8;

        if (v->rep < u->rep) {
          u->rep = v->rep;
          u->link = NULL;
        }
        // TODO

        (void)SINK;
T8:  // New strong component
        (void)ROOT;
        (void)a;
};
}  // namespace

void
SGB::Run( )
{
        taocp::Run( this );
};
}  // namespace taocp
