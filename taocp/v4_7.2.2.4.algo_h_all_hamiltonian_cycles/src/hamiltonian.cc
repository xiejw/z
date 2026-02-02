// TAOCP Vol F8A, Section 7.2.2.4 Page 14
//
// forge:v1
//
#include "hamiltonian.h"

#include <assert.h>

namespace taocp {
namespace {
void
Run( size_t n )
{
        (void)n;
}
}  // namespace

HamiltonianGraph::HamiltonianGraph( size_t n ) : n( n )
{
        this->mem = (size_t *)malloc( sizeof( size_t ) * 2 * n );
        this->EV  = this->mem;
        this->EU  = this->mem + n;
}

HamiltonianGraph::~HamiltonianGraph( ) { free( this->mem ); }

void
HamiltonianGraph::RunAlgoH( )
{
        Run( this->n );
}
}  // namespace taocp
