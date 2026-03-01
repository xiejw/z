// forge:v1
#include "two_sat_solver.h"

#include <assert.h>

#define DEBUG 0
#define DEBUG_PRINTF \
        if ( DEBUG ) INFO

namespace taocp {

// Reserve 2n in the graph. n for the variables, n for the complements of
// variables.
TwoSatSolver::TwoSatSolver( size_t num_vars ) : n( num_vars ), g{ 2 * num_vars }
{
}

void
TwoSatSolver::AddKromClause( size_t v_id, size_t u_id )
{
        assert( v_id >= 0 && v_id < 2 * this->n );
        assert( u_id >= 0 && u_id < 2 * this->n );

        auto *comp_v = this->g.GetVertex( this->GetComplementId( v_id ) );
        comp_v->arcs.push_back( this->g.GetVertex( u_id ) );

        auto *comp_u = this->g.GetVertex( this->GetComplementId( u_id ) );
        comp_u->arcs.push_back( this->g.GetVertex( v_id ) );
}

bool
TwoSatSolver::CheckSatisfiability( )
{
        this->g.RunAlgoT( );
        auto ids = g.GetComponentIdsAfterAlgoT( );

        if ( DEBUG ) {
                DEBUG_PRINTF( "\nN= %d\n", int( this->n ) );
                for ( size_t i = 0; i < ids->size( ); i++ ) {
                        DEBUG_PRINTF( "%d => comp %d", int( i ),
                                      int( ids->at( i ) ) );
                }
        }

        // Exercise 54 (Page 86): Instead of checking whether a variable and
        // its complement exist in one component for all variables, we could
        // simply check the first variable only. If its complement is in the
        // same component, the 2SAT is not satisfiability.
        const size_t        SENT = 2 * this->n;
        std::vector<size_t> leader_for_components( ids->size( ), SENT );

        for ( size_t vertex_id = 0; vertex_id < SENT; vertex_id++ ) {
                const size_t component_id = ( *ids )[vertex_id];
                const size_t current_leader =
                    leader_for_components[component_id];

                DEBUG_PRINTF(
                    "CHECK component_id = %2d vertex_id = %2d current_leader = "
                    "%2d",
                    int( component_id ), int( vertex_id ),
                    int( current_leader ) );

                if ( current_leader == SENT ) {
                        leader_for_components[component_id] = vertex_id;
                        continue;
                }

                if ( this->GetCanonicalId( current_leader ) ==
                     this->GetCanonicalId( vertex_id ) ) {
                        return false;
                }
        }

        return true;
}
}  // namespace taocp
