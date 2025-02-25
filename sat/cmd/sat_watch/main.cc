#include <cstring>
#include <print>

#include <algos/sat_watch.h>
#include <eve/base/error.h>
#include <eve/base/log.h>

using eve::algos::sat::C;
using eve::algos::sat::PrintLiterals;
using eve::algos::sat::WatchSolver;

static auto RunProgramByWatchSolver( ) -> void;

int
main( int argc, char **argv )
{
        if ( argc > 2 ) panic( "expect 0 or 1 argument. got %d", argc - 1 );

        if ( argc == 1 || 0 == strcmp( argv[1], "watch" ) ) {
                RunProgramByWatchSolver( );
                return 0;
        }

        panic( "expert one argument as {watch}. got %s", argv[1] );
}

auto
RunProgramByWatchSolver( ) -> void
{
        std::print( "==== --- Demo Problem for Algorithm B --- ===\n" );
        std::print( "==== --- Satisfiability by Watching   --- ===\n" );
        WatchSolver sov{ /*num_literals=*/3, /*num_causes=*/4 };

        sov.EmitClause( { { ( 1 ) } } );
        sov.EmitClause( {
            { 1, C( 2 ) }
        } );
        sov.EmitClause( {
            { 2, C( 3 ) }
        } );
        sov.EmitClause( {
            { 2, 3 }
        } );
        sov.DebugPrint( );

        if ( auto res = sov.Search( ); res ) {
                std::print( "good\n" );
                PrintLiterals( res.value( ) );
        } else {
                std::print( "bad\n" );
        }
}
