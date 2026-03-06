// forge:skip
#include <string.h>

#include "log.h"
#include "sat_cdcl.h"
#include "sat_literal.h"

using taocp::C;
using taocp::CDCLSolver;
using taocp::literal_t;
using taocp::PrintClause;

namespace {
// auto
// PrintAndEmitClause( WatchSolver *sov, std::initializer_list<literal_t> lits )
// {
//         PrintClause( lits.size( ), std::data( lits ) );
//         sov->EmitClause( lits );
// }

auto
RunSolver( ) -> void
{
        INFO( "==== --- Demo Problem for Algorithm C --- ===" );
        INFO( "==== --- Satisfiability by CDCL --------- ===" );
        // WatchSolver sov{ /*num_literals=*/3, /*num_causes=*/4 };

        // INFO( "Emit Clauses:" );
        // PrintAndEmitClause( &sov, { 1 } );
        // PrintAndEmitClause( &sov, { 1, C( 2 ) } );
        // PrintAndEmitClause( &sov, { 2, C( 3 ) } );
        // PrintAndEmitClause( &sov, { 2, 3 } );

        //// INFO( "Debug Print:" );
        //// sov.dump_debug_info( );

        // if ( auto res = sov.SearchOneSolution( ); res ) {
        //         INFO( "Satisfiable!!!" );
        //         INFO( "Result:" );
        //         PrintClause( res.value( ).size( ), res.value( ).data( ) );
        // } else {
        //         PANIC( "Unsatisfiable" );
        // }
}
}  // namespace

int
main( )
{
        RunSolver( );
        return 0;
}
