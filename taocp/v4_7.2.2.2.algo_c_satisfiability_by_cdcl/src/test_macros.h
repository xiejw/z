// vim: ft=cpp
//
// forge:v3
//
// === --- Test Code ------------------------------------------------------- ===
//
// History
// - [2026-02-25]: V3 Test not return NULL.
// - [2026-01-26]: V2 Add central registry.
// - [2026-01-26]: V1 Simple EXPECT_TRUE.
//
// This file offers the macros to make testing easier.
//
#include <stdio.h>
#include <stdlib.h>

//
// === --- Assert Macros --------------------------------------------------- ===
//
// Usage:
//
//     EXPECT_TRUE(foo_return_one() == 1, "expect 1");
//

#define EXPECT_TRUE( eq_condition, msg ) \
        _EXPECT_TRUE_IMPL( eq_condition, msg, __FILE__, __LINE__ )

//
// === --- Registry -------------------------------------------------------- ===
//
// Usage
//
//     FORGE_TEST( test_foo )
//     {
//             return;
//     }
//
//     int
//     main( )
//     {
//             forge::test_suite_run( );
//     }
//
#include <functional>
#include <vector>

namespace forge {
inline std::vector<std::pair<const char *, std::function<void( )>>> &
test_suite_registry( )
{
        static std::vector<std::pair<const char *, std::function<void( )>>>
            funcs;
        return funcs;
}

inline void
test_suite_run( )
{
        for ( auto &f : test_suite_registry( ) ) {
                printf( "[ RUN ] %s", f.first );
                f.second( );
                printf( ".\n" );
        }
        printf( "Test passed.\n" );
}
}  // namespace forge

#define FORGE_TEST( fn_name )                                      \
        void fn_name( );                                           \
        struct fn_name##_registrar {                               \
                fn_name##_registrar( )                             \
                {                                                  \
                        ::forge::test_suite_registry( ).push_back( \
                            { #fn_name, fn_name } );               \
                }                                                  \
        };                                                         \
        static fn_name##_registrar fn_name##_instance;             \
        void                       fn_name( )

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
