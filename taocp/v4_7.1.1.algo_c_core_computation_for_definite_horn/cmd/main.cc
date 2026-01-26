#include <stdio.h>

#include "horn.h"
#include "log.h"
#include "test_macros.h"

namespace {
using namespace taocp;

FORGE_TEST( test_new )
{
        struct horn *h = horn_new( /*num_variables=*/3 );
        horn_free( h );
        return NULL;
}

FORGE_TEST( test_core_no_progress )
{
        struct horn *h = horn_new( /*num_variables=*/3 );

        // 2
        // 2
        // !0 || 1
        //
        // expected: core: 2

        horn_add_clause( h, 2, 0, NULL );
        horn_add_clause( h, 2, 0, NULL );
        HORN_ADD_CLAUSE_HELPER( h, 1, 1, 0 );

        horn_core_compute( h );

        EXPECT_TRUE( 0 == horn_is_var_in_core( h, 0 ), "prop_0 core" );
        EXPECT_TRUE( 0 == horn_is_var_in_core( h, 1 ), "prop_1 core" );
        EXPECT_TRUE( 1 == horn_is_var_in_core( h, 2 ), "prop_2 core" );

        horn_free( h );
        return NULL;
}

FORGE_TEST( test_core_simple )
{
        struct horn *h = horn_new( /*num_variables=*/5 );

        // 2
        // !2 || 1
        // !3 || 0
        // !1 || !4 || 0
        // 4
        //
        // expected: core: 0, 1, 2, 4
        HORN_ADD_CLAUSE_WO_HYPOTHESES( h, 2 );
        HORN_ADD_CLAUSE_HELPER( h, 1, 1, 2 );
        HORN_ADD_CLAUSE_HELPER( h, 0, 1, 3 );
        HORN_ADD_CLAUSE_HELPER( h, 0, 2, 1, 4 );
        HORN_ADD_CLAUSE_WO_HYPOTHESES( h, 4 );
        horn_add_clause( h, 4, 0, NULL );

        horn_core_compute( h );

        EXPECT_TRUE( 1 == horn_is_var_in_core( h, 0 ), "prop_0 core" );
        EXPECT_TRUE( 1 == horn_is_var_in_core( h, 1 ), "prop_1 core" );
        EXPECT_TRUE( 1 == horn_is_var_in_core( h, 2 ), "prop_2 core" );
        EXPECT_TRUE( 0 == horn_is_var_in_core( h, 3 ), "prop_3 core" );
        EXPECT_TRUE( 1 == horn_is_var_in_core( h, 4 ), "prop_4 core" );

        horn_free( h );
        return NULL;
}

FORGE_TEST( test_solve_easy )
{
        struct horn *h = horn_new( /*num_variables=*/3 );

        // 2
        // 2
        // !0 || 1
        //
        // expected: yes

        horn_add_clause( h, 2, 0, NULL );
        horn_add_clause( h, 2, 0, NULL );
        HORN_ADD_CLAUSE_HELPER( h, 1, 1, 0 );

        horn_core_compute( h );
        EXPECT_TRUE( true == horn_is_satisfiable( h ), "yes" );
        EXPECT_TRUE( 0 == horn_is_var_in_core( h, 0 ), "prop_0 core" );
        EXPECT_TRUE( 0 == horn_is_var_in_core( h, 1 ), "prop_1 core" );
        EXPECT_TRUE( 1 == horn_is_var_in_core( h, 2 ), "prop_2 core" );

        horn_free( h );
        return NULL;
}

FORGE_TEST( test_solve_no_solu )
{
        struct horn *h = horn_new( /*num_variables=*/3 );

        // 2
        // 2
        // !0 || 1
        // 0
        // !1
        //
        // expected: no

        horn_add_clause( h, 2, 0, NULL );
        horn_add_clause( h, 2, 0, NULL );
        HORN_ADD_CLAUSE_HELPER( h, 1, 1, 0 );
        horn_add_clause( h, 0, 0, NULL );
        HORN_ADD_CLAUSE_HELPER( h, -1, 1, 1 );

        horn_core_compute( h );

        EXPECT_TRUE( false == horn_is_satisfiable( h ), "no" );

        horn_free( h );
        return NULL;
}

FORGE_TEST( test_solve_simple )
{
        struct horn *h = horn_new( /*num_variables=*/5 );

        // 2
        // !2 || 1
        // !3 || 0
        // !1 || !4 || 0
        // 4
        //
        // expected: yes
        horn_add_clause( h, 2, 0, NULL );
        HORN_ADD_CLAUSE_HELPER( h, 1, 1, 2 );
        HORN_ADD_CLAUSE_HELPER( h, 0, 1, 3 );
        HORN_ADD_CLAUSE_HELPER( h, 0, 2, 1, 4 );
        horn_add_clause( h, 4, 0, NULL );

        horn_core_compute( h );

        EXPECT_TRUE( true == horn_is_satisfiable( h ), "yes" );
        EXPECT_TRUE( 1 == horn_is_var_in_core( h, 0 ), "prop_0 core" );
        EXPECT_TRUE( 1 == horn_is_var_in_core( h, 1 ), "prop_1 core" );
        EXPECT_TRUE( 1 == horn_is_var_in_core( h, 2 ), "prop_2 core" );
        EXPECT_TRUE( 0 == horn_is_var_in_core( h, 3 ), "prop_3 core" );
        EXPECT_TRUE( 1 == horn_is_var_in_core( h, 4 ), "prop_4 core" );

        horn_free( h );
        return NULL;
}

FORGE_TEST( test_solve_simple_not_def )
{
        struct horn *h = horn_new( /*num_variables=*/5 );

        // 2
        // !2 || 1
        // !3
        // !1 || !4 || 0
        // 4
        //
        // expected: yes
        horn_add_clause( h, 2, 0, NULL );
        HORN_ADD_CLAUSE_HELPER( h, 1, 1, 2 );
        HORN_ADD_CLAUSE_HELPER( h, -1, 1, 3 );
        HORN_ADD_CLAUSE_HELPER( h, 0, 2, 1, 4 );
        horn_add_clause( h, 4, 0, NULL );
        horn_core_compute( h );

        EXPECT_TRUE( true == horn_is_satisfiable( h ), "yes" );

        horn_free( h );
        return NULL;
}

FORGE_TEST( test_solve_multiple )
{
        struct horn *h = horn_new( /*num_variables=*/3 );

        // 2
        // !2 || 1
        // !2 || 0
        //
        // expected: yes
        HORN_ADD_CLAUSE_WO_HYPOTHESES( h, 2 );
        HORN_ADD_CLAUSE_HELPER( h, 1, 1, 2 );
        HORN_ADD_CLAUSE_HELPER( h, 0, 1, 2 );

        horn_core_compute( h );
        EXPECT_TRUE( true == horn_is_satisfiable( h ), "yes" );

        EXPECT_TRUE( 1 == horn_is_var_in_core( h, 0 ), "prop_0 core" );
        EXPECT_TRUE( 1 == horn_is_var_in_core( h, 1 ), "prop_1 core" );
        EXPECT_TRUE( 1 == horn_is_var_in_core( h, 2 ), "prop_2 core" );

        horn_free( h );
        return NULL;
}

FORGE_TEST( test_solve_multiple_no_solu )
{
        struct horn *h = horn_new( /*num_variables=*/3 );

        // 2
        // !2 || 1
        // !2
        //
        // expected: no
        HORN_ADD_CLAUSE_WO_HYPOTHESES( h, 2 );
        HORN_ADD_CLAUSE_HELPER( h, 1, 1, 2 );
        HORN_ADD_CLAUSE_HELPER( h, -1, 1, 2 );
        horn_core_compute( h );

        EXPECT_TRUE( false == horn_is_satisfiable( h ), "no solution" );
        EXPECT_TRUE( 0 == horn_is_var_in_core( h, 0 ), "prop_0 core" );
        EXPECT_TRUE( 1 == horn_is_var_in_core( h, 1 ), "prop_1 core" );
        EXPECT_TRUE( 1 == horn_is_var_in_core( h, 2 ), "prop_2 core" );

        horn_free( h );
        return NULL;
}
}  // namespace

int
main( )
{
        forge::test_suite_run( );
}
