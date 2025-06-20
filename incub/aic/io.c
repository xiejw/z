#include "io.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#define READ_BUF_SIZE 4096

struct io_reader {
        struct ctx *ctx; /* Unowned */
        int         fd;
        char        line[READ_BUF_SIZE];
        error_t     err;
        size_t      idx; /* The next char to be consided for next call. */
        size_t end; /* The end marker in line buffer which has been filled. */
};

error_t
io_reader_open( struct ctx *ctx, const char *name,
                _OUT_ struct io_reader **out )
{
        struct io_reader *p = malloc( sizeof( *p ) );
        assert( p != NULL );
        int fd = open( name, O_RDONLY );
        if ( fd < 0 ) {
                EMIT_ERROR_NOTE( ctx, "failed to open file %s: %s", name,
                                 strerror( errno ) );
                return EIO;
        }
        p->ctx = ctx;
        p->fd  = fd;
        p->err = OK;
        p->idx = 0;
        p->end = 0;
        *out   = p;
        return OK;
}
void
io_reader_close( struct io_reader *p )
{
        if ( p == NULL ) return;
        close( p->fd );
        free( p );
}

error_t
io_reader_nextline( struct io_reader *r, _OUT_ char **buf, _OUT_ size_t *size,
                    _OUT_ _NULLABLE_ int *partial )
{
        if ( r->err != OK ) return r->err;

        if ( r->idx == r->end ) {
                r->idx = 0;
                r->end = 0;
                goto fill_buf;
        }

        goto scan_line;

fill_buf:
        /* The assumption is from r->idx to r->end, there is no newline. */
        assert( r->idx <= r->end );
        assert( r->end < READ_BUF_SIZE );
        ssize_t c = read( r->fd, r->line + r->end, READ_BUF_SIZE - r->end );
        if ( c == 0 ) {
                r->err = EEOF;
                if ( r->end > r->idx ) {
                        // The last line in the buf.
                        *buf  = r->line + r->idx;
                        *size = r->end - r->idx;
                        if ( partial != NULL ) *partial = 0;
                        return OK;
                }
                return EEOF;
        }

        if ( c < 0 ) {
                EMIT_ERROR_NOTE( r->ctx, "failed to read files: %s",
                                 strerror( errno ) );
                r->err = EIO;
                return EIO;
        }

        r->end += (size_t)c;
        assert( r->end <= READ_BUF_SIZE );
        goto scan_line;

scan_line:
        assert( r->idx < r->end );

        for ( size_t i = r->idx; i < r->end; i++ ) {
                if ( r->line[i] == '\n' ) {
                        *buf  = r->line + r->idx;
                        *size = i - r->idx;
                        if ( partial != NULL ) *partial = 0;
                        r->idx = i + 1;
                        return OK;
                }
        }

        /* In current line buffer, there is no newline. */
        if ( r->idx == 0 && r->end < READ_BUF_SIZE ) {
                goto fill_buf;
        }

        if ( r->end < READ_BUF_SIZE ) {
                assert( r->idx > 0 );
                goto move_line_and_fillbuf;
        }

        /* if we hit here. then no NEWLINE is found. */
        if ( r->idx == 0 && r->end == READ_BUF_SIZE ) {
                *buf  = r->line;
                *size = READ_BUF_SIZE;
                if ( partial != NULL ) *partial = 1;
                r->idx = 0;
                r->end = 0;
                return OK;
        }

        assert( r->idx > 0 && r->end == READ_BUF_SIZE );
        goto move_line_and_fillbuf;

move_line_and_fillbuf:

        assert( r->idx > 0 );
        size_t size_cpy = r->end - r->idx;
        for ( size_t i = 0; i < size_cpy; i++ ) {
                r->line[i] = r->line[r->idx + i];
        }
        r->idx = 0;
        r->end = size_cpy;
        goto fill_buf;
}

/* === --- Test Code -------------------------------------------------------- */

#ifdef AIC_TEST_H_
#include <stdio.h>

#include <adt/sds.h>

int
main( void )
{
        /* Get the content from io_reader. */
        struct ctx       *ctx = ctx_new( );
        struct io_reader *r;
        error_t           err = io_reader_open( ctx, "io.c", &r );
        sds_t             got = sds_empty( );

        char  *buf;
        size_t size;
        while ( ( err = io_reader_nextline( r, &buf, &size, NULL ) ) == OK ) {
                sds_cat_printf( &got, "%.*s\n", (int)size, buf );
        }
        io_reader_close( r );
        ctx_free( ctx );

        /* Get the content from posix read. */
        sds_t expected = sds_empty( );
        int   fd       = open( "io.c", O_RDONLY );
        assert( fd >= 0 );
        ssize_t c;
        char    iobuf[4096];
        while ( ( c = read( fd, iobuf, 4096 ) ) > 0 ) {
                sds_cat_printf( &expected, "%.*s", (int)c, iobuf );
        }
        close( fd );

        /* Compare */
        assert( 0 == strcmp( got, expected ) );

        sds_free( got );
        sds_free( expected );
        printf( "Test passed.\n" );
        return 0;
}

#endif /* AIC_TEST_H_ */
