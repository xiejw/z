#include "testing/testing.h"

// opus
#include <algos/horn.h>

static char *
test_new( void )
{
    struct horn *h = hornNew( /*num_props=*/3 );
    hornFree( h );
    return NULL;
}

static char *
test_new_clause( void )
{
    struct horn *h = hornNew( /*num_props=*/3 );

    hornAddClause( h, 2, 0, NULL );
    hornAddClause( h, 2, 0, NULL );
    hornAddClause( h, 1, 1, (int[]){ 0 } );

    ASSERT_TRUE( "s_top_id", 1 == h->stack_top );
    ASSERT_TRUE( "s_top", 2 == h->stack[0] );

    ASSERT_TRUE( "prop_0_truth", 0 == h->props[0].truth );
    ASSERT_TRUE( "prop_1_truth", 0 == h->props[1].truth );
    ASSERT_TRUE( "prop_2_truth", 1 == h->props[2].truth );

    ASSERT_TRUE( "clause_2_conclusion", 1 == h->clauses->clause.conclusion );
    ASSERT_TRUE( "clause_2_num_hypo", 1 == h->clauses->clause.num_hypotheses );

    ASSERT_TRUE( "clause_1_conclusion",
                 2 == h->clauses->next->clause.conclusion );
    ASSERT_TRUE( "clause_1_num_hypo",
                 0 == h->clauses->next->clause.num_hypotheses );

    ASSERT_TRUE( "clause_0_conclusion",
                 2 == h->clauses->next->next->clause.conclusion );
    ASSERT_TRUE( "clause_0_num_hypo",
                 0 == h->clauses->next->next->clause.num_hypotheses );

    hornFree( h );
    return NULL;
}

static char *
test_core_no_progress( void )
{
    struct horn *h = hornNew( /*num_props=*/3 );

    // 2
    // 2
    // !0 || 1
    //
    // expected: core: 2

    hornAddClause( h, 2, 0, NULL );
    hornAddClause( h, 2, 0, NULL );
    hornAddClause( h, 1, 1, (int[]){ 0 } );

    hornCore( h );

    ASSERT_TRUE( "s_top_id", 0 == h->stack_top );
    ASSERT_TRUE( "prop_0_truth", 0 == h->props[0].truth );
    ASSERT_TRUE( "prop_1_truth", 0 == h->props[1].truth );
    ASSERT_TRUE( "prop_2_truth", 1 == h->props[2].truth );

    hornFree( h );
    return NULL;
}

static char *
test_core_simple( void )
{
    struct horn *h = hornNew( /*num_props=*/5 );

    // 2
    // !2 || 1
    // !3 || 0
    // !1 || !4 || 0
    // 4
    //
    // expected: core: 0, 1, 2, 4
    HORN_ADD_C0( h, 2 );
    HORN_ADD_C1( h, 1, 2 );
    HORN_ADD_C1( h, 0, 3 );
    HORN_ADD_C2( h, 0, 1, 4 );
    HORN_ADD_C0( h, 4 );

    hornCore( h );

    ASSERT_TRUE( "s_top_id", 0 == h->stack_top );
    ASSERT_TRUE( "prop_0_truth", 1 == h->props[0].truth );
    ASSERT_TRUE( "prop_1_truth", 1 == h->props[1].truth );
    ASSERT_TRUE( "prop_2_truth", 1 == h->props[2].truth );
    ASSERT_TRUE( "prop_3_truth", 0 == h->props[3].truth );
    ASSERT_TRUE( "prop_4_truth", 1 == h->props[4].truth );

    hornFree( h );
    return NULL;
}

static char *
test_core_multiple( void )
{
    struct horn *h = hornNew( /*num_props=*/3 );

    // 2
    // !2 || 1
    // !2 || 0
    //
    // expected: core: 0, 1, 2
    hornAddClause( h, 2, 0, NULL );
    hornAddClause( h, 1, 1, (int[]){ 2 } );
    hornAddClause( h, 0, 1, (int[]){ 2 } );

    hornCore( h );

    ASSERT_TRUE( "s_top_id", 0 == h->stack_top );
    ASSERT_TRUE( "prop_0_truth", 1 == h->props[0].truth );
    ASSERT_TRUE( "prop_1_truth", 1 == h->props[1].truth );
    ASSERT_TRUE( "prop_2_truth", 1 == h->props[2].truth );

    hornFree( h );
    return NULL;
}

static char *
test_solve_easy( void )
{
    struct horn *h = hornNew( /*num_props=*/3 );

    // 2
    // 2
    // !0 || 1
    //
    // expected: yes

    hornAddClause( h, 2, 0, NULL );
    hornAddClause( h, 2, 0, NULL );
    hornAddClause( h, 1, 1, (int[]){ 0 } );

    ASSERT_TRUE( "yes", 1 == hornSolve( h ) );

    hornFree( h );
    return NULL;
}

static char *
test_solve_no_solu( void )
{
    struct horn *h = hornNew( /*num_props=*/3 );

    // 2
    // 2
    // !0 || 1
    // 0
    // !1
    //
    // expected: no

    hornAddClause( h, 2, 0, NULL );
    hornAddClause( h, 2, 0, NULL );
    hornAddClause( h, 1, 1, (int[]){ 0 } );
    hornAddClause( h, 0, 0, NULL );
    hornAddClause( h, -1, 1, (int[]){ 1 } );

    ASSERT_TRUE( "no", 0 == hornSolve( h ) );

    hornFree( h );
    return NULL;
}

static char *
test_solve_simple( void )
{
    struct horn *h = hornNew( /*num_props=*/5 );

    // 2
    // !2 || 1
    // !3 || 0
    // !1 || !4 || 0
    // 4
    //
    // expected: yes
    hornAddClause( h, 2, 0, NULL );
    hornAddClause( h, 1, 1, (int[]){ 2 } );
    hornAddClause( h, 0, 1, (int[]){ 3 } );
    hornAddClause( h, 0, 2, (int[]){ 1, 4 } );
    hornAddClause( h, 4, 0, NULL );

    ASSERT_TRUE( "yes", 1 == hornSolve( h ) );

    hornFree( h );
    return NULL;
}

static char *
test_solve_simple_not_indef( void )
{
    struct horn *h = hornNew( /*num_props=*/5 );

    // 2
    // !2 || 1
    // !3
    // !1 || !4 || 0
    // 4
    //
    // expected: yes
    hornAddClause( h, 2, 0, NULL );
    hornAddClause( h, 1, 1, (int[]){ 2 } );
    hornAddClause( h, -1, 1, (int[]){ 3 } );
    hornAddClause( h, 0, 2, (int[]){ 1, 4 } );
    hornAddClause( h, 4, 0, NULL );

    ASSERT_TRUE( "yes", 1 == hornSolve( h ) );

    hornFree( h );
    return NULL;
}

static char *
test_solve_multiple( void )
{
    struct horn *h = hornNew( /*num_props=*/3 );

    // 2
    // !2 || 1
    // !2 || 0
    //
    // expected: yes
    HORN_ADD_C0( h, 2 );
    HORN_ADD_C1( h, 1, 2 );
    HORN_ADD_C1( h, 0, 2 );

    ASSERT_TRUE( "yes", 1 == hornSolve( h ) );

    hornFree( h );
    return NULL;
}

static char *
test_solve_multiple_no_solu( void )
{
    struct horn *h = hornNew( /*num_props=*/3 );

    // 2
    // !2 || 1
    // !2
    //
    // expected: no
    HORN_ADD_C0( h, 2 );
    HORN_ADD_C1( h, 1, 2 );
    HORN_ADD_C1( h, HORN_NO_CONC, 2 );

    ASSERT_TRUE( "no", 0 == hornSolve( h ) );

    hornFree( h );
    return NULL;
}

DECLARE_TEST_SUITE( algos_horn )
{
    RUN_TEST( test_new );
    RUN_TEST( test_new_clause );
    RUN_TEST( test_core_no_progress );
    RUN_TEST( test_core_simple );
    RUN_TEST( test_core_multiple );
    RUN_TEST( test_solve_easy );
    RUN_TEST( test_solve_no_solu );
    RUN_TEST( test_solve_simple );
    RUN_TEST( test_solve_simple_not_indef );
    RUN_TEST( test_solve_multiple );
    RUN_TEST( test_solve_multiple_no_solu );
    return NULL;
}
