#include <cstring>

#include "sat.h"

#include <zion/zion.h>

using eos::sat::C;
using eos::sat::literal_t;
using eos::sat::print_clause_literals;
using eos::sat::WatchSolver;

namespace {
auto
print_and_emit_clause( WatchSolver *sov, std::span<const literal_t> lits )
    -> void
{
        print_clause_literals( lits );
        sov->emit_clause( lits );
}

auto
run_watch_solver( ) -> void
{
        INFO( "==== --- Demo Problem for Algorithm B --- ===" );
        INFO( "==== --- Satisfiability by Watching   --- ===" );
        WatchSolver sov{ /*num_literals=*/3, /*num_causes=*/4 };

        INFO( "Emit Clauses:" );
        print_and_emit_clause( &sov, { { ( 1 ) } } );
        print_and_emit_clause( &sov, {
                                         { 1, C( 2 ) }
        } );
        print_and_emit_clause( &sov, {
                                         { 2, C( 3 ) }
        } );
        print_and_emit_clause( &sov, {
                                         { 2, 3 }
        } );

        INFO( "Debug Print:" );
        sov.DebugPrint( );

        if ( auto res = sov.search( ); res ) {
                INFO( "Satisfiable!!!" );
                INFO( "Result:" );
                print_clause_literals( res.value( ) );
        } else {
                WARN( "Unsatisfiable" );
        }
}
}  // namespace

int
main( int argc, char **argv )
{
        if ( argc > 2 ) PANIC( "Expect 0 or 1 argument. got {}", argc - 1 );

        if ( argc == 1 || 0 == strcmp( argv[1], "watch" ) ) {
                run_watch_solver( );
                return 0;
        }

        PANIC( "Expert one argument as 'watch'. got {}", argv[1] );
}
