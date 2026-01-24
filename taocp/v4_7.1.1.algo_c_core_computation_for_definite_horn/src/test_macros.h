// vim: ft=cpp
//
// === --- Test Code ------------------------------------------------------- ===
//
// This file offers the macros to make testing easier.
//
#include <stdio.h>
#include <stdlib.h>

#define EXPECT_TRUE( eq_condition, msg ) \
        _EXPECT_TRUE_IMPL( eq_condition, msg, __FILE__, __LINE__ )

//
// === --- Implementation Code --------------------------------------------- ===
//
#define _EXPECT_TRUE_IMPL( eq_condition, msg, file, line )                 \
        do {                                                               \
                if ( !( eq_condition ) ) {                                 \
                        printf( "Assertion failed. %s:%d\n", file, line ); \
                        printf( msg "\n" );                                \
                        exit( -1 );                                        \
                }                                                          \
        } while ( 0 )
