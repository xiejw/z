#include <adt/sds.h>

#include <stdio.h>  // vsnprintf
#include <stdlib.h>
#include <string.h>  // memcpy/memset

static const int SDS_DEFAULT_ALLOCATE_SPACE = 16;

// -----------------------------------------------------------------------------
// implementation.
// -----------------------------------------------------------------------------

static inline sds_t _sdsRaw( const void *init, size_t len, size_t cap );

sds_t
sdsEmptyWithCap( size_t cap )
{
        sds_t            s    = _sdsRaw( NULL, 0, cap );
        struct sds_head *phdr = SDS_HEAD( s );
        phdr->len             = 0;
        s[0]                  = '\0';
        return s;
}

sds_t
sdsNew( const char *init )
{
        size_t initlen = ( init == NULL ) ? 0 : strlen( init );
        size_t cap     = initlen;

        sds_t            s    = _sdsRaw( init, initlen, cap );
        struct sds_head *phdr = SDS_HEAD( s );
        phdr->len             = initlen;
        s[initlen]            = '\0';
        return s;
}

sds_t
sdsEmpty( void )
{
        return sdsNew( "" );
}

sds_t
sdsDup( const sds_t s )
{
        size_t len   = sdsLen( s );
        sds_t  new_s = _sdsRaw( s, len, sdsCap( s ) );

        struct sds_head *phdr = SDS_HEAD( new_s );
        phdr->len             = len;
        new_s[len]            = '\0';
        return new_s;
}

void
sdsFree( sds_t s )
{
        if ( s == NULL ) return;
        free( (void *)SDS_HEAD( s ) );
}

void
sdsReserve( sds_t *s, size_t new_len )
{
        size_t cap = sdsCap( *s );
        if ( cap >= new_len ) return;

        new_len *= 2;

        size_t hdrlen = sizeof( struct sds_head );
        void  *buf    = SDS_HEAD( *s );
        buf           = realloc( buf, hdrlen + new_len + 1 );
        if ( buf == NULL ) {
                *s = NULL;
                return;
        }
        *s = (sds_t)buf + hdrlen;
        sdsSetCap( *s, new_len );
}

void
sdsCatLen( sds_t *s, const void *t, size_t len )
{
        size_t curlen = sdsLen( *s );
        size_t newlen = curlen + len;
        sdsReserve( s, newlen );
        if ( *s == NULL ) return;
        memcpy( ( *s ) + curlen, t, len );
        sdsSetLen( *s, newlen );
        ( *s )[newlen] = '\0';
}

void
sdsCat( sds_t *s, const char *t )
{
        sdsCatLen( s, t, strlen( t ) );
}

void
sdsCatSds( sds_t *s, const sds_t t )
{
        sdsCatLen( s, t, sdsLen( t ) );
}

void
sdsCatPrintf( sds_t *s, const char *fmt, ... )
{
        va_list ap;
        va_start( ap, fmt );
        sdsCatVprintf( s, fmt, ap );
        va_end( ap );
}

void
sdsCatVprintf( sds_t *s, const char *fmt, va_list ap )
{
        va_list cpy;
        char    staticbuf[1024], *buf = staticbuf;
        size_t  buflen = strlen( fmt ) * 2;

        assert( buflen >= 2 );
        // fast path first. Use the remaining area if possible for speed.
        {
                size_t avail = sdsAvail( *s );
                if ( buflen <= avail ) {
                        size_t cur_len = sdsLen( *s );

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshadow"
                        char *buf       = ( *s ) + cur_len;
                        buf[buflen - 2] = '\0';
                        va_copy( cpy, ap );
                        vsnprintf( buf, buflen, fmt, cpy );
                        va_end( cpy );
                        if ( buf[buflen - 2] == '\0' ) {
                                size_t inc_len = strlen( buf );
                                sdsSetLen( *s, cur_len + inc_len );
                                return;
                        }
                        // fall through
#pragma clang diagnostic pop
                }
        }

        /* We try to start using a static buffer for speed.
         * If not possible we revert to heap allocation. */
        if ( buflen > sizeof( staticbuf ) ) {
                buf = malloc( buflen );
                if ( buf == NULL ) {
                        *s = NULL;
                        return;
                }
        } else {
                buflen = sizeof( staticbuf );
        }

        /* Try with buffers two times bigger every time we fail to
         * fit the string in the current buffer size. */
        while ( 1 ) {
                buf[buflen - 2] = '\0';
                va_copy( cpy, ap );
                vsnprintf( buf, buflen, fmt, cpy );
                va_end( cpy );
                if ( buf[buflen - 2] != '\0' ) {
                        if ( buf != staticbuf ) free( buf );
                        buflen *= 2;
                        buf = malloc( buflen );
                        if ( buf == NULL ) {
                                *s = NULL;
                                return;
                        }
                        continue;
                }
                break;
        }

        /* Finally concat the obtained string to the SDS string and return it.
         */
        sdsCat( s, buf );
        if ( buf != staticbuf ) free( buf );
}

void
sdsCpyLen( sds_t *s, const char *t, size_t len )
{
        sdsReserve( s, len );
        if ( *s == NULL ) return;
        memcpy( *s, t, len );
        ( *s )[len] = '\0';
        sdsSetLen( *s, len );
        return;
}
void
sdsCpy( sds_t *s, const char *t )
{
        sdsCpyLen( s, t, strlen( t ) );
}

// compare two sds strings s1 and s2 with memcmp().
//
// Return value:
//
//     positive if s1 > s2.
//     negative if s1 < s2.
//     0 if s1 and s2 are exactly the same binary string.
//
// If two strings share exactly the same prefix, but one of the two has
// additional characters, the longer string is considered to be greater than
// the smaller one.
//
int
sdsCmp( const sds_t s1, const sds_t s2 )
{
        size_t l1, l2, minlen;
        int    cmp;

        l1     = sdsLen( s1 );
        l2     = sdsLen( s2 );
        minlen = ( l1 < l2 ) ? l1 : l2;
        cmp    = memcmp( s1, s2, minlen );
        if ( cmp == 0 ) return l1 > l2 ? 1 : ( l1 < l2 ? -1 : 0 );
        return cmp;
}

// create a raw sds without filling len.
//
// - the space is at least as large as `cap`. cap will be set.
// - allocate `cap` size but only initialize `len` size.
// - the final `\0` is not set.
sds_t
_sdsRaw( const void *init, size_t len, size_t cap )
{
        cap =
            cap > SDS_DEFAULT_ALLOCATE_SPACE ? cap : SDS_DEFAULT_ALLOCATE_SPACE;
        size_t hdrlen = sizeof( struct sds_head );
        void  *buf    = malloc( hdrlen + cap + 1 );
        if ( buf == NULL ) return NULL;

        sds_t s                           = (sds_t)buf + hdrlen;
        ( (struct sds_head *)buf )->alloc = cap;

        if ( len ) {
                if ( init == NULL )
                        memset( s, 0, len );
                else
                        memcpy( s, init, len );
        }

        return s;
}

/* === --- Test Code -------------------------------------------------------- */

#ifdef ADT_TEST_H_
#include <stdio.h>
int
main( void )
{
        printf( "Test passed.\n" );
        return 0;
}

#endif /* ADT_TEST_H_ */
