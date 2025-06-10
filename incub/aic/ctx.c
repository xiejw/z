#include "ctx.h"

#include <stdio.h>
#include <string.h>

#include <adt/sds.h>

#define CTX_OUTPUT_FMT "[file %10s: line %4d] "

struct ctx {
        sds_t error_msg;
};

struct ctx *
ctx_new( void )
{
        struct ctx *p = malloc( sizeof( *p ) );
        assert( p != NULL );
        p->error_msg = sds_empty( );
        return p;
}

void
ctx_free( struct ctx *p )
{
        if ( p == NULL ) return;
        sds_free( p->error_msg );
        free( p );
}

void
ctx_emit_note( struct ctx *p, enum ctx_note_level note_level, const char *file,
               int line, const char *fmt, ... )
{
#ifdef NDEBUG
        (void)note_level;
#endif
        assert( note_level == CTX_NOTE_ERROR );
        va_list args;
        va_start( args, fmt );
        sds_cat_printf( &p->error_msg, CTX_OUTPUT_FMT, file, line );
        sds_cat_vprintf( &p->error_msg, fmt, args );
        if ( fmt[strlen( fmt ) - 1] != '\n' ) sds_cat( &p->error_msg, "\n" );
        va_end( args );
}

void
ctx_dump_error_note( struct ctx *p )
{
        fprintf( stderr, "%s", p->error_msg );
}

const char *
ctx_get_error_note( struct ctx *p )
{
        return p->error_msg;
}

void
ctx_emit_log( struct ctx *p, enum ctx_log_level level, const char *file,
              int line, const char *fmt, ... )
{
        (void)p;
        FILE   *stream = level == CTX_LOG_ERROR ? stderr : stdout;
        va_list args;
        va_start( args, fmt );
        fprintf( stream, CTX_OUTPUT_FMT, file, line );
        vfprintf( stream, fmt, args );
        if ( fmt[strlen( fmt ) - 1] != '\n' ) fprintf( stream, "\n" );
        va_end( args );
}
