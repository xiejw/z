#include "util.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>  // NULL
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

double
time_ms_since_epoch( void )
{
        struct timeval tv;

        gettimeofday( &tv, NULL );

        return ( (double)( tv.tv_sec ) * 1000.0 +
                 (double)( tv.tv_usec ) / 1000.0 );
}

void
bytecode_store_u64( vec_t( byte ) * pcode, u64 v )
{
        size_t size = vec_size( *pcode );
        vec_reserve( pcode, size + 8 );
        byte *ptr = ( *pcode ) + size;
        ptr[0]    = (byte)( v );
        ptr[1]    = (byte)( v >> 8 );
        ptr[2]    = (byte)( v >> 16 );
        ptr[3]    = (byte)( v >> 24 );
        ptr[4]    = (byte)( v >> 32 );
        ptr[5]    = (byte)( v >> 40 );
        ptr[6]    = (byte)( v >> 48 );
        ptr[7]    = (byte)( v >> 56 );
        vec_set_size( *pcode, size + 8 );
}

u64
bytecode_load_u64( byte *ptr )
{
        return (u64)( ptr[0] ) | ( (u64)( ptr[1] ) << 8 ) |
               ( (u64)( ptr[2] ) << 16 ) | ( (u64)( ptr[3] ) << 24 ) |
               ( (u64)( ptr[4] ) << 32 ) | ( (u64)( ptr[5] ) << 40 ) |
               ( (u64)( ptr[6] ) << 48 ) | ( (u64)( ptr[7] ) << 56 );
}

/* One time initialized lookup table to avoid switch cost during base64
 * decoding.
 *
 * See base64_lookup_tbl_init why 1 is here.
 */
static char BASE64_LOOKUP_TBL[256] = { 1 };

/* One time initialize the base64 lookup table. */
static void
base64_lookup_tbl_init( void )
{
        /* Non one means this has been initialized. */
        if ( BASE64_LOOKUP_TBL[0] != 1 ) return;

        char idx = 0;
        for ( char i = 'A'; i <= 'Z'; i++ ) {
                BASE64_LOOKUP_TBL[(int)i] = idx++;
        }
        assert( idx == 26 );

        for ( char i = 'a'; i <= 'z'; i++ ) {
                BASE64_LOOKUP_TBL[(int)i] = idx++;
        }
        assert( idx == 26 + 26 );

        for ( char i = '0'; i <= '9'; i++ ) {
                BASE64_LOOKUP_TBL[(int)i] = idx++;
        }
        assert( idx == 26 + 26 + 10 );

        BASE64_LOOKUP_TBL[(int)'+'] = idx++;
        BASE64_LOOKUP_TBL[(int)'/'] = idx++;
        assert( idx == 64 );

        BASE64_LOOKUP_TBL[(int)'='] = 0;

        BASE64_LOOKUP_TBL[0] = 0; /* Record init is done. */
}

/* Algorithm
 * - Check https://en.wikipedia.org/wiki/Base64 for reference.
 * - Unpack 4 chars each time. Fill the bits into the 3 chars of the
 *   output.
 * - The '=' padding is ignored as it just becomes a NULL terminator.
 */
char *
base64_decode( char *buf, size_t len )
{
        base64_lookup_tbl_init( );
        assert( len > 0 && len % 4 == 0 );
        char *output = malloc( len / 4 * 3 + 1 );

        /* Once lookup up table is initialized, all valid chars, except '=' or
         * 'A', should have non zero value. */
#define CHECK_VALID_BASE64_CH( idx )                          \
        assert( buf[( idx )] == '=' || buf[( idx )] == 'A' || \
                BASE64_LOOKUP_TBL[(int)buf[( idx )]] != 0 )

        size_t output_idx = 0;
        for ( size_t i = 0; i < len; i += 4 ) {
                CHECK_VALID_BASE64_CH( i );
                CHECK_VALID_BASE64_CH( i + 1 );
                CHECK_VALID_BASE64_CH( i + 2 );
                CHECK_VALID_BASE64_CH( i + 3 );

                char a = BASE64_LOOKUP_TBL[(int)buf[i]];
                char b = BASE64_LOOKUP_TBL[(int)buf[i + 1]];
                char c = BASE64_LOOKUP_TBL[(int)buf[i + 2]];
                char d = BASE64_LOOKUP_TBL[(int)buf[i + 3]];

                output[output_idx++] = (char)( ( a << 2 ) | ( b >> 4 ) );
                output[output_idx++] =
                    (char)( ( ( b & 0xF ) << 4 ) | ( c >> 2 ) );
                output[output_idx++] = (char)( ( ( c & 0x3 ) << 6 ) | d );
        }

#undef CHECK_VALID_BASE64_CH

        output[output_idx] = '\0';
        return output;
}

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
#include <string.h>

#include <adt/sds.h>

static void
test_base64( void )
{
        char *res;
        res = base64_decode( "TQ==", 4 );
        assert( 0 == strcmp( "M", res ) );
        free( res );

        res = base64_decode( "TWE=", 4 );
        assert( 0 == strcmp( "Ma", res ) );
        free( res );

        res = base64_decode( "bGlnaHQgd29yaw==", 16 );
        assert( 0 == strcmp( "light work", res ) );
        free( res );

        res = base64_decode( "bGlnaHQgd29yay4=", 16 );
        assert( 0 == strcmp( "light work.", res ) );
        free( res );
}

static void
test_io( void )
{
        /* Get the content from io_reader. */
        struct ctx       *ctx = ctx_new( );
        struct io_reader *r;
        error_t           err = io_reader_open( ctx, "util.c", &r );
        assert( err == OK );
        sds_t got = sds_empty( );

        char  *buf;
        size_t size;
        while ( ( err = io_reader_nextline( r, &buf, &size, NULL ) ) == OK ) {
                sds_cat_printf( &got, "%.*s\n", (int)size, buf );
        }
        io_reader_close( r );
        ctx_free( ctx );

        /* Get the content from posix read. */
        sds_t expected = sds_empty( );
        int   fd       = open( "util.c", O_RDONLY );
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
}

int
main( void )
{
        test_base64( );
        test_io( );

        printf( "Test passed.\n" );
        return 0;
}

#endif  // AIC_TEST_H_
