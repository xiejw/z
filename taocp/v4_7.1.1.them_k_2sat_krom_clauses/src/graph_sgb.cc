// TAOCP Vol F12A, Section 7.4.1.2 Page 10
//
// forge:v2
//
#include "graph_sgb.h"

#include <assert.h>
#include <stdio.h>
#include <algorithm>

#define DEBUG 0
#define DEBUG_PRINTF \
        if ( DEBUG ) printf

namespace taocp {
namespace {

void
RunAlgoT( std::vector<SGBNode> *vertices, std::vector<size_t> *component_ids )
{
        // Only used for assignment and comparison.
        //
        // NOTE: REP(SENT) is not set as 0. See special treatment in T8 how it
        // is handled. Otherwise, SENT->rep is operated on a invalid memory
        // slot.
        //

        SGBNode *const SENT         = &vertices->data( )[vertices->size( )];
        SGBNode *const START        = &vertices->data( )[0];
        const size_t   NUM_VERTICES = vertices->size( );

        // Aux vars used by the Algorithm
        size_t           p;     // Current LOW. <= NUM_VERTICES. See T3.
        SGBNode         *SINK;  // Top of the stack of unsettled vertices.
        SGBNode         *ROOT;  // Root of current tree. See T2.
        SGBNode         *v;     // Current working node. See T3.
        SGBNode::ArcIter a;     // Current working arc. See T3.
        SGBNode         *w;     // Current working tree root. See T2.

        // Dual usage:
        // - Tip of the current arc. See T5.
        // - When hit T8, u is the parent of v. T9 uses it.
        //
        SGBNode *u;

T1:  // Initialize
        for ( auto &v : *vertices ) {
                v.parent = NULL;
                v.arc    = v.arcs.begin( );
        }
        w    = SENT;
        p    = 0;
        SINK = SENT;

T2:  // Done?

        // Invariant:
        // - w is the current tree root.
        // - At the begin of T2, w is not visited yet.
        //

        do {
                if ( w == START ) goto Exit;  // g->vertices is 0 based.
                w--;
        } while ( w->parent != NULL );

        v         = w;
        v->parent = SENT;
        ROOT      = v;

T3:  // Begin to explore from v;

        // Invariant:
        // - Explore v for the first time.
        // - The arc a is set to the beginning of the arcs (not arc).
        // - Set rep with p and increase p.
        //
        p++;
        assert( p <= NUM_VERTICES );

        a       = v->arcs.begin( );
        v->rep  = p;
        v->link = SENT;  // See (11)

T4:  // Done with v

        // The arc a must belong to v or v.arcs.end()
        assert( a >= v->arcs.begin( ) && a <= v->arcs.end( ) );
        if ( a == v->arcs.end( ) ) goto T7;

T5:  // Visit next arc: v -> u

        // Invariant: v -> u
        //
        u = *a;
        a++;

T6:  // if u is new move to it

        // Invariant: u is the Tip of the arc a: v -> u.
        //
        if ( u->parent == NULL ) {  // Move to u as new v.
                u->parent = v;
                v->arc    = a;  // Store the state. T3 will overwrite a.
                v         = u;
                goto T3;
        }

        // TODO; Why???
        if ( u == ROOT && p == NUM_VERTICES ) {  // Last component
                while ( v != ROOT ) {
                        // Link into unsettled stack nodes.
                        v->link = SINK;
                        SINK    = v;

                        v = v->parent;
                }

                // Invariant:
                // - v is the ROOT now.
                // - u is used for other purposes now. u is the v's parent
                //   before goto T8 as T9, follows T8, uses u's value (the
                //   SENT).
                //
                u = SENT;
                goto T8;
        }

        // Mature the arc. See paragraph before Theorem T (F12A, Page 9).
        if ( u->rep < v->rep ) {
                v->rep  = u->rep;
                v->link = NULL;  // See (11)
        }

        goto T4;

T7:  // Finish with v;

        // Invariant:
        // - v, the current working node, is finished.
        // - u will be used for other purposes in this code block. u is the v's
        //   parent before goto T8/T9 as T9, follows T8, uses u's value.
        //
        u = v->parent;  // u must be set before goto T8 as it is used in T9.

        if ( v->link == SENT ) goto T8;  // See (11)

        // Mature the arc. See paragraph before Theorem T (F12A, Page 9).
        if ( v->rep < u->rep ) {
                u->rep  = v->rep;
                u->link = NULL;  // See (11)
        }

        // Link into unsettled stack nodes.
        v->link = SINK;
        SINK    = v;

        assert( u == v->parent );
        goto T9;

T8:  // New strong component

        // Invariant:
        // - v and its unsettled descendants form a strong component that will
        //   be represented by v.
        // - u must be the v's parent.
        assert( u == v->parent );
        assert( v >= START );

        {
                // This is diff from the algorithm to leverage pointer
                // arithmetic in c++. Value must >= NUM_VERTICES.  Idea is
                // same.
                //
                // See (12).
                size_t component_rep = NUM_VERTICES + size_t( v - START );
                while ( true ) {
                        size_t sink_rep = SINK == SENT ? 0 : SINK->rep;
                        if ( sink_rep < v->rep ) break;

                        SINK->rep = component_rep;
                        SINK      = SINK->link;
                }
                v->rep = component_rep;
        }

T9:  // Tree done?

        // Invariant
        // - u is the parent of v.

        assert( u == v->parent );
        if ( u == SENT ) goto T2;  // Tree done. See T2 of the condition.

        // Otherwise, backtrack and restore v be the working node and a be the
        // next arc. u will be set in T5.
        //
        v = u;
        a = v->arc;
        goto T4;

Exit:  // Exit routine

        // Invariant
        // -- All nodes are in some components.
        assert( std::all_of(
            vertices->begin( ), vertices->end( ),
            [=]( auto &node ) { return node.rep >= NUM_VERTICES; } ) );

        // Fill Component Ids
        SGBNode *ptr   = &( vertices->data( )[0] );
        size_t   total = vertices->size( );
        component_ids->reserve( total );
        for ( size_t i = 0; i < total; i++ ) {
                component_ids->push_back( size_t( ptr[i].rep - NUM_VERTICES ) );
        }
};

void
DebugCheckVertexInvariant( std::vector<SGBNode> *vertices )
{
#ifdef NDEBUG
        (void)vertices;
        return;
#else

        for ( auto &n : *vertices ) {
                DEBUG_PRINTF( "\nNode\n" );
                bool found = false;
                if ( n.arcs.empty( ) ) {
                        DEBUG_PRINTF( "Empty\n" );
                        found = true;
                        goto arc_loop;
                }

                for ( size_t i = 0; i < n.arcs.size( ); i++ ) {
                        SGBNode *u = n.arcs[i];
                        for ( size_t j = 0; j < vertices->size( ); j++ ) {
                                SGBNode *m = &vertices->at( j );
                                DEBUG_PRINTF( "compare %lx vs %lx\n",
                                              (unsigned long)u,
                                              (unsigned long)m );
                                if ( u == m ) {
                                        found = true;
                                        goto arc_loop;
                                }
                        }
                }
                DEBUG_PRINTF( "Hmmm \n" );
        arc_loop:
                assert( found );
        }

#endif
}

}  // namespace

void
SGBGraph::RunAlgoT( )
{
        assert( this->num_vertices_expected == this->vertices.size( ) );
        DebugCheckVertexInvariant( &this->vertices );
        taocp::RunAlgoT( &this->vertices, &this->component_ids );
};
}  // namespace taocp
