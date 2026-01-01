// vim: ft=cpp
//
// Thread safe std::format like logging library, i.e., INFO, DEBUG, WARN.
//
// In addition, the PANIC macro is used to abort the execution with messages.
// It prints out file name and line number.
//
#pragma once

#include <cstdio>
#include <print>

// === --- Logging --------------------------------------------------------- ===
//
#define ZION_COLOR_CYAN  "\033[1;36m"
#define ZION_COLOR_RED   "\033[1;31m"
#define ZION_COLOR_PURPL "\033[1;35m"
#define ZION_COLOR_YELW  "\033[1;33m"
#define ZION_COLOR_GREEN "\033[1;32m"
#define ZION_COLOR_RESET "\033[0m"

// Panic in case things go wrong.
#define PANIC( ... ) _ZION_PANIC( __FILE__, __LINE__, __VA_ARGS__ )

#define WARN( ... )                                                    \
        _ZION_LOG( stderr, ZION_COLOR_PURPL "[WARN]" ZION_COLOR_RESET, \
                   __VA_ARGS__ )
#define INFO( ... )                                                    \
        _ZION_LOG( stdout, ZION_COLOR_GREEN "[INFO]" ZION_COLOR_RESET, \
                   __VA_ARGS__ )
#define DEBUG( ... ) _ZION_LOG( stdout, "[DEBG]", __VA_ARGS__ )

#define _ZION_LOG( stream, PREFIX, fmt, ... )                       \
        do {                                                        \
                time_t _t = time( NULL );                           \
                char   _zion_buf[26];                               \
                ctime_r( &_t, _zion_buf );                          \
                _zion_buf[24] = 0; /* Suppress new line */          \
                                                                    \
                /* std::print is thread safe. */                    \
                std::print( ( stream ), PREFIX "[{}] " fmt "\n",    \
                            _zion_buf __VA_OPT__(, ) __VA_ARGS__ ); \
        } while ( 0 )

#define _ZION_PANIC( FILE, LINE, ... )                                         \
        do {                                                                   \
                fflush( stdout );                                              \
                _ZION_LOG(                                                     \
                    stderr, ZION_COLOR_RED "[<-- PANIC -->]" ZION_COLOR_RESET, \
                    "File at " ZION_COLOR_CYAN "{}" ZION_COLOR_RESET           \
                    " && Line at " ZION_COLOR_YELW "{}" ZION_COLOR_RESET,      \
                    FILE, LINE );                                              \
                _ZION_LOG( stderr,                                             \
                           ZION_COLOR_RED "[<-- PANIC -->]" ZION_COLOR_RESET,  \
                           __VA_ARGS__ );                                      \
                std::abort( );                                                 \
        } while ( 0 )
