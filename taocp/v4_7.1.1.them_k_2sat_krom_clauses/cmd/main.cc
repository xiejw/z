// === --- 2SAT Satisfiability Vol 4A 7.1.1 Theorm K Page 62 --------------- ===
//
// === --- TLDR
//
// Key Ideas:
//
//     Form 2SAT as directed graph. Then, partition the graph with strong
//     compnents. Leverage Theorm K to conclude satisfiability.
//
// === --- Dependencies
//
// The code base uses strong component algorithm.
//
// forge:v1
#include <stdio.h>

#include "graph_sgb.h"
#include "log.h"
#include "test_macros.h"

#define DEBUG 0
#define DEBUG_PRINTF \
        if ( DEBUG ) INFO

namespace taocp {
struct TwoSatSolver {
      private:
        size_t   n;
        SGBGraph g;

      public:
        TwoSatSolver( size_t num_vars );

      public:
        void AddKromClause( size_t v_id, size_t u_id );
        bool CheckSatisfiability( );

        size_t GetCompVarId( size_t var_id )
        {
                return var_id >= n ? var_id - n : var_id + n;
        }
        size_t GetRawVarId( size_t var_id )
        {
                return var_id >= n ? var_id - n : var_id;
        }
};

TwoSatSolver::TwoSatSolver( size_t num_vars ) : n( num_vars ), g{ 2 * num_vars }
{
}

void
TwoSatSolver::AddKromClause( size_t v_id, size_t u_id )
{
        assert( v_id >= 0 && v_id < 2 * this->n );
        assert( u_id >= 0 && u_id < 2 * this->n );

        auto *comp_v = this->g.GetVertex( this->GetCompVarId( v_id ) );
        comp_v->arcs.push_back( this->g.GetVertex( u_id ) );

        auto *comp_u = this->g.GetVertex( this->GetCompVarId( u_id ) );
        comp_u->arcs.push_back( this->g.GetVertex( v_id ) );
}

bool
TwoSatSolver::CheckSatisfiability( )
{
        this->g.RunAlgoT( );
        auto ids = g.GetComponentIdsAfterAlgoT( );

        if ( DEBUG ) {
                DEBUG_PRINTF( "\n" );
                for ( size_t i = 0; i < ids->size( ); i++ ) {
                        DEBUG_PRINTF( "%d => comp %d", int( i ),
                                      int( ids->at( i ) ) );
                }
        }

        // Exercise 54 (Page 86).
        for ( size_t component_id = 0; component_id < ids->size( );
              component_id++ ) {
                const size_t SENT           = 2 * this->n;
                size_t       current_leader = SENT;

                for ( size_t vertex_id = 0; vertex_id < SENT; vertex_id++ ) {
                        if ( ( *ids )[vertex_id] != component_id ) {
                                continue;
                        }

                        if ( current_leader == SENT ) {
                                current_leader = vertex_id;
                        } else if ( this->GetRawVarId( current_leader ) ==
                                    this->GetRawVarId( vertex_id ) ) {
                                return false;
                        }
                }
        }
        return true;
}

}  // namespace taocp

namespace {
using namespace taocp;

FORGE_TEST( test_single_node_sat )
{
        // a || C(a)          C(a) -> C(a)
        TwoSatSolver s{ 1 };
        const size_t a = 0;
        s.AddKromClause( a, s.GetCompVarId( a ) );
        bool sat = s.CheckSatisfiability( );

        EXPECT_TRUE( sat, "true" );

        return NULL;
}

FORGE_TEST( test_single_node_constradict )
{
        // a || a               C(a) -> a
        // C(a) || C(a)         a -> C(a)
        TwoSatSolver s{ 1 };
        const size_t a = 0;
        s.AddKromClause( a, a );
        s.AddKromClause( s.GetCompVarId( a ), s.GetCompVarId( a ) );
        bool sat = s.CheckSatisfiability( );

        EXPECT_TRUE( !sat, "not sat" );

        return NULL;
}

FORGE_TEST( test_two_nodes )
{
        // C(a) || C(b)          a -> C(b)
        //   b  || C(a)          C(b) -> C(a)
        TwoSatSolver s{ 2 };
        const size_t a = 0;
        const size_t b = 1;
        s.AddKromClause( s.GetCompVarId( a ), s.GetCompVarId( b ) );
        s.AddKromClause( b, s.GetCompVarId( a ) );
        bool sat = s.CheckSatisfiability( );

        EXPECT_TRUE( sat, "true" );

        return NULL;
}

}  // namespace

int
main( )
{
        forge::test_suite_run( );
}
