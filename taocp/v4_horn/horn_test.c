#include "horn.h"

#include <stdio.h>

void horn_core_compute( struct horn *h );

// === --- Test Code ------------------------------------------------------- ===

#define PANIC( )                     \
        do {                         \
                printf( "panic\n" ); \
                exit( -1 );          \
        } while ( 0 )

#define EXPECT_TRUE( eq_condition, msg ) \
        _EXPECT_TRUE_IMPL( eq_condition, msg, __FILE__, __LINE__ )

#define _EXPECT_TRUE_IMPL( eq_condition, msg, file, line )                 \
        do {                                                               \
                if ( !( eq_condition ) ) {                                 \
                        printf( "Assertion failed. %s:%d\n", file, line ); \
                        printf( msg "\n" );                                \
                        PANIC( );                                          \
                }                                                          \
        } while ( 0 )

static char *
test_new( void )
{
        struct horn *h = horn_new( /*num_props=*/3 );
        horn_free( h );
        return NULL;
}

static char *
test_core_no_progress( void )
{
        struct horn *h = horn_new( /*num_props=*/3 );

        // 2
        // 2
        // !0 || 1
        //
        // expected: core: 2

        horn_add_clause( h, 2, 0, NULL );
        horn_add_clause( h, 2, 0, NULL );
        horn_add_clause( h, 1, 1, (int[]){ 0 } );

        horn_core_compute( h );

        EXPECT_TRUE( 0 == horn_is_prop_in_core( h, 0 ), "prop_0 core" );
        EXPECT_TRUE( 0 == horn_is_prop_in_core( h, 1 ), "prop_1 core" );
        EXPECT_TRUE( 1 == horn_is_prop_in_core( h, 2 ), "prop_2 core" );

        horn_free( h );
        return NULL;
}

static char *
test_core_simple( void )
{
        struct horn *h = horn_new( /*num_props=*/5 );

        // 2
        // !2 || 1
        // !3 || 0
        // !1 || !4 || 0
        // 4
        //
        // expected: core: 0, 1, 2, 4
        HORN_ADD_CLAUSE_WO_HYPOTHESES( h, 2 );
        HORN_ADD_CLAUSE( h, 1, 1, 2 );
        HORN_ADD_CLAUSE( h, 0, 1, 3 );
        HORN_ADD_CLAUSE( h, 0, 2, 1, 4 );
        HORN_ADD_CLAUSE_WO_HYPOTHESES( h, 4 );
        horn_add_clause( h, 4, 0, NULL );

        horn_core_compute( h );

        EXPECT_TRUE( 1 == horn_is_prop_in_core( h, 0 ), "prop_0 core" );
        EXPECT_TRUE( 1 == horn_is_prop_in_core( h, 1 ), "prop_1 core" );
        EXPECT_TRUE( 1 == horn_is_prop_in_core( h, 2 ), "prop_2 core" );
        EXPECT_TRUE( 0 == horn_is_prop_in_core( h, 3 ), "prop_3 core" );
        EXPECT_TRUE( 1 == horn_is_prop_in_core( h, 4 ), "prop_4 core" );

        horn_free( h );
        return NULL;
}

static char *
test_solve_easy( void )
{
        struct horn *h = horn_new( /*num_props=*/3 );

        // 2
        // 2
        // !0 || 1
        //
        // expected: yes

        horn_add_clause( h, 2, 0, NULL );
        horn_add_clause( h, 2, 0, NULL );
        horn_add_clause( h, 1, 1, (int[]){ 0 } );

        EXPECT_TRUE( OK == horn_search( h ), "yes" );
        EXPECT_TRUE( 0 == horn_is_prop_in_core( h, 0 ), "prop_0 core" );
        EXPECT_TRUE( 0 == horn_is_prop_in_core( h, 1 ), "prop_1 core" );
        EXPECT_TRUE( 1 == horn_is_prop_in_core( h, 2 ), "prop_2 core" );

        horn_free( h );
        return NULL;
}

static char *
test_solve_no_solu( void )
{
        struct horn *h = horn_new( /*num_props=*/3 );

        // 2
        // 2
        // !0 || 1
        // 0
        // !1
        //
        // expected: no

        horn_add_clause( h, 2, 0, NULL );
        horn_add_clause( h, 2, 0, NULL );
        horn_add_clause( h, 1, 1, (int[]){ 0 } );
        horn_add_clause( h, 0, 0, NULL );
        horn_add_clause( h, -1, 1, (int[]){ 1 } );

        EXPECT_TRUE( ENOTEXIST == horn_search( h ), "no" );

        horn_free( h );
        return NULL;
}

static char *
test_solve_simple( void )
{
        struct horn *h = horn_new( /*num_props=*/5 );

        // 2
        // !2 || 1
        // !3 || 0
        // !1 || !4 || 0
        // 4
        //
        // expected: yes
        horn_add_clause( h, 2, 0, NULL );
        horn_add_clause( h, 1, 1, (int[]){ 2 } );
        horn_add_clause( h, 0, 1, (int[]){ 3 } );
        horn_add_clause( h, 0, 2, (int[]){ 1, 4 } );
        horn_add_clause( h, 4, 0, NULL );

        EXPECT_TRUE( OK == horn_search( h ), "yes" );
        EXPECT_TRUE( 1 == horn_is_prop_in_core( h, 0 ), "prop_0 core" );
        EXPECT_TRUE( 1 == horn_is_prop_in_core( h, 1 ), "prop_1 core" );
        EXPECT_TRUE( 1 == horn_is_prop_in_core( h, 2 ), "prop_2 core" );
        EXPECT_TRUE( 0 == horn_is_prop_in_core( h, 3 ), "prop_3 core" );
        EXPECT_TRUE( 1 == horn_is_prop_in_core( h, 4 ), "prop_4 core" );

        horn_free( h );
        return NULL;
}

static char *
test_solve_simple_not_def( void )
{
        struct horn *h = horn_new( /*num_props=*/5 );

        // 2
        // !2 || 1
        // !3
        // !1 || !4 || 0
        // 4
        //
        // expected: yes
        horn_add_clause( h, 2, 0, NULL );
        horn_add_clause( h, 1, 1, (int[]){ 2 } );
        horn_add_clause( h, -1, 1, (int[]){ 3 } );
        horn_add_clause( h, 0, 2, (int[]){ 1, 4 } );
        horn_add_clause( h, 4, 0, NULL );

        EXPECT_TRUE( OK == horn_search( h ), "expect solution" );

        horn_free( h );
        return NULL;
}

static char *
test_solve_multiple( void )
{
        struct horn *h = horn_new( /*num_props=*/3 );

        // 2
        // !2 || 1
        // !2 || 0
        //
        // expected: yes
        HORN_ADD_CLAUSE_WO_HYPOTHESES( h, 2 );
        HORN_ADD_CLAUSE( h, 1, 1, 2 );
        HORN_ADD_CLAUSE( h, 0, 1, 2 );

        EXPECT_TRUE( OK == horn_search( h ), "yes" );

        EXPECT_TRUE( 1 == horn_is_prop_in_core( h, 0 ), "prop_0 core" );
        EXPECT_TRUE( 1 == horn_is_prop_in_core( h, 1 ), "prop_1 core" );
        EXPECT_TRUE( 1 == horn_is_prop_in_core( h, 2 ), "prop_2 core" );

        horn_free( h );
        return NULL;
}

static char *
test_solve_multiple_no_solu( void )
{
        struct horn *h = horn_new( /*num_props=*/3 );

        // 2
        // !2 || 1
        // !2
        //
        // expected: no
        HORN_ADD_CLAUSE_WO_HYPOTHESES( h, 2 );
        HORN_ADD_CLAUSE( h, 1, 1, 2 );
        HORN_ADD_CLAUSE_WO_CONCLUSION( h, 1, 2 );

        EXPECT_TRUE( OK != horn_search( h ), "no solution" );
        EXPECT_TRUE( 0 == horn_is_prop_in_core( h, 0 ), "prop_0 core" );
        EXPECT_TRUE( 1 == horn_is_prop_in_core( h, 1 ), "prop_1 core" );
        EXPECT_TRUE( 1 == horn_is_prop_in_core( h, 2 ), "prop_2 core" );

        horn_free( h );
        return NULL;
}

int
main( void )
{
        test_new( );
        test_core_no_progress( );
        test_core_simple( );
        test_solve_easy( );
        test_solve_no_solu( );
        test_solve_simple( );
        test_solve_simple_not_def( );
        test_solve_multiple( );
        test_solve_multiple_no_solu( );
        printf( "Test passed.\n" );
}
