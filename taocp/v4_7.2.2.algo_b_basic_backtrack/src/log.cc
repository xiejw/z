// forge:v1
#include "log.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

namespace base {

#define ZION_COLOR_CYAN  "\033[1;36m"
#define ZION_COLOR_RED   "\033[1;31m"
#define ZION_COLOR_PURPL "\033[1;35m"
#define ZION_COLOR_YELW  "\033[1;33m"
#define ZION_COLOR_GREEN "\033[1;32m"
#define ZION_COLOR_RESET "\033[0m"

void
PanicImpl( const char *file, int line, const char *fmt, ... )
{
        /* Get the time stamp string. */
        time_t _t = time( NULL );
        char   _zion_buf[26];
        ctime_r( &_t, _zion_buf );
        _zion_buf[24] = 0; /* Suppress new line */

        /* Print panic title. */
        printf( ZION_COLOR_RED
                "<-- PANIC --> [%s F@%s:L@%3d]\n" ZION_COLOR_RESET,
                _zion_buf, file, line );

        /* Log msg. */
        va_list args;
        va_start( args, fmt );
        vprintf( fmt, args );
        if ( fmt[strlen( fmt ) - 1] != '\n' ) printf( "\n" );
        va_end( args );
        fflush( stdout );
        exit( 1 );
}

namespace {

void
LogImpl( const char *file, int line, const char *pfx, const char *fmt,
         va_list args )
{
        /* Get the time stamp string. */
        time_t _t = time( NULL );
        char   _zion_buf[26];
        ctime_r( &_t, _zion_buf );
        _zion_buf[24] = 0; /* Suppress new line */

        /* Print log prefix. */
        printf( "%s[%s F@%s:L@%3d] ", pfx, _zion_buf, file, line );

        /* Log msg. */
        vprintf( fmt, args );
        if ( fmt[strlen( fmt ) - 1] != '\n' ) printf( "\n" );
}
}  // namespace

void
InfoImpl( const char *file, int line, const char *fmt, ... )
{
        va_list args;
        va_start( args, fmt );
        LogImpl( file, line, ZION_COLOR_GREEN "[INFO]" ZION_COLOR_RESET, fmt,
                 args );
        va_end( args );
}

void
WarnImpl( const char *file, int line, const char *fmt, ... )
{
        va_list args;
        va_start( args, fmt );
        LogImpl( file, line, ZION_COLOR_PURPL "[WARN]" ZION_COLOR_RESET, fmt,
                 args );
        va_end( args );
}

}  // namespace base
