#include "two_sat_solver.h"

#include <assert.h>

namespace taocp {

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

        // Exercise 54 (Page 86): Instead of checking whether a variable and
        // its complement exist in one component for all variables, we could
        // simply check the first variable only. If its complement is in the
        // same component, the 2SAT is not satisfiability.
        for ( size_t component_id = 0; component_id < ids->size( );
              component_id++ ) {
                // Check each component.
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
