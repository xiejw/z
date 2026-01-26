// vim: ft=cpp
//
// === --- Test Code ------------------------------------------------------- ===
//
// History
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

#define EXPECT_TRUE( eq_condition, msg ) \
        _EXPECT_TRUE_IMPL( eq_condition, msg, __FILE__, __LINE__ )

//
// === --- Registry -------------------------------------------------------- ===
//
#include <functional>
#include <vector>

namespace tests {
inline std::vector<std::pair<const char *, std::function<char *( )>>> &
registry( )
{
        static std::vector<std::pair<const char *, std::function<char *( )>>>
            funcs;
        return funcs;
}

inline void
run( )
{
        for ( auto &f : registry( ) ) {
                printf( "[ RUN ] %s", f.first );
                f.second( );
                printf( ".\n" );
        }
}
}  // namespace tests

#define TEST( fn_name )                                                        \
        char *fn_name( );                                                      \
        struct fn_name##_registrar {                                           \
                fn_name##_registrar( )                                         \
                {                                                              \
                        tests::registry( ).push_back( { #fn_name, fn_name } ); \
                }                                                              \
        };                                                                     \
        static fn_name##_registrar fn_name##_instance;                         \
        char                      *fn_name( )

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
