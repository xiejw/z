#include <stdio.h>

// opus
#include <algos/horn.h>

int
main( void )
{
    printf( "horn clause satisfiability\n\n" );
    printf( "    clause: a_2\n" );
    printf( "    clause: a_1 || !a_2\n" );
    printf( "    clause:        !a_3\n" );
    printf( "    clause: a_0 || !a_1 || !a_4\n" );
    printf( "    clause: a_4\n" );
    printf( "\n" );

    struct horn *h = hornNew( /*num_props=*/5 );

    HORN_ADD_C0( h, 2 );
    HORN_ADD_C1( h, 1, 2 );
    HORN_ADD_C1( h, HORN_NO_CONC, 3 );
    HORN_ADD_C2( h, 0, 1, 4 );
    HORN_ADD_C0( h, 4 );

    int result = hornSolve( h );
    printf( "%s\n", result ? "satisfiable" : "not satisfiable" );

    hornFree( h );
    return 0;
}
