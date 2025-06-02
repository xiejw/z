#include <testing/test_main.h>

int
main( void )
{
    // ---------------------------------------------------------------------
    // Adds all suites.
    //
    // Convenstion is for foo, a test suite fn run_foo_suite is called. For
    // customized case, use ADD_SUITE_NAME_AND_FN.

    // algos
    ADD_SUITE( algos_horn );  // src/algos/horn_test.c

    // ---------------------------------------------------------------------
    // Runs all suites and reports.
    int suites_failed = run_all_suites( );
    return suites_failed ? -1 : 0;
}
