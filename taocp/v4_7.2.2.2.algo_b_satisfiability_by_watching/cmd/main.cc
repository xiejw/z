#include <string.h>

#include "log.h"
#include "sat_literal.h"
#include "sat_watch.h"

using taocp::C;
using taocp::literal_t;
using taocp::PrintClause;
using taocp::WatchSolver;

namespace {
auto
PrintAndEmitClause( WatchSolver *sov, std::initializer_list<literal_t> lits )
{
        PrintClause( lits.size( ), std::data( lits ) );
        sov->EmitClause( lits );
}

auto
RunSolver( ) -> void
{
        INFO( "==== --- Demo Problem for Algorithm B --- ===" );
        INFO( "==== --- Satisfiability by Watching   --- ===" );
        WatchSolver sov{ /*num_literals=*/3, /*num_causes=*/4 };

        INFO( "Emit Clauses:" );
        PrintAndEmitClause( &sov, { 1 } );
        PrintAndEmitClause( &sov, { 1, C( 2 ) } );
        PrintAndEmitClause( &sov, { 2, C( 3 ) } );
        PrintAndEmitClause( &sov, { 2, 3 } );

        // INFO( "Debug Print:" );
        // sov.dump_debug_info( );

        if ( auto res = sov.SearchOneSolution( ); res ) {
                INFO( "Satisfiable!!!" );
                INFO( "Result:" );
                PrintClause( res.value( ).size( ), res.value( ).data( ) );
        } else {
                PANIC( "Unsatisfiable" );
        }
}
}  // namespace

int
main( )
{
        RunSolver( );
        return 0;
}
