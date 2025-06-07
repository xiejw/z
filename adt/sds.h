#ifndef ADT_SDS_H_
#define ADT_SDS_H_

#include <assert.h>
#include <stdarg.h>  // va_list
#include <stdlib.h>

#include <adt/types.h>

// === --- design ---------------------------------------------------------- ===
//
// +----+------+------
// |len |alloc |buf
// +----+------+------
//             |
//  -2   -1     `-- sds_t
//
// - Fast assessing:   sds_t is basically the c string ('\0' terminated).
// - Dynamic growth:   If the space is not enough, buf will be expanded to hold
//                     more elements in future.
// - Fast CatPrintf:   Use remaing space to printf whenever possible.
//
// ----------------------
// invariance and caveats
// ----------------------
//   - The following should be true:
//
//     strlen(sds) = sdsLen(sds) <= sdsCap(sds).
//
//   - So to use sdsSetLen, sdsSetCap, sdsIncLen, caller must set the \0'
//     correctly and maintain the invariance above.
// -----------------------------------------------------------------------------

// === --- APIs ------------------------------------------------------------ ===
typedef char *sds_t;

struct sds_head {
        size_t len;
        size_t alloc;
};

// Allocator and free fns.
//
// - sdsNew           copies a NULL-able str and allocates enough space for that
// - sdsEmpty         is basically sdsNew("");
// - sdsEmptyWithCap  good for creating a sds with known cap.
// - sdsDup           dups a sds.
sds_t sdsNew( const char *init );
sds_t sdsEmpty( void );
sds_t sdsEmptyWithCap( size_t cap );
sds_t sdsDup( const sds_t s );
void  sdsFree( sds_t s );

#define sdsClear( s ) _sdsClear( s )  // clear to 0-len str
#define sdsLen( s )   _sdsLen( s )
#define sdsCap( s )   _sdsCap( s )
#define sdsAvail( s ) _sdsAvail( s )
#define sdsSetLen( s, l ) \
        _sdsSetLen( ( s ), ( l ) )  // caller checks invariance
#define sdsSetCap( s, l ) \
        _sdsSetCap( ( s ), ( l ) )  // caller checks invariance
#define sdsIncLen( s, inc ) \
        _sdsIncLen( ( s ), ( inc ) )  // caller checks invariance

// allocte enough space (typically larger) for the new_len.
void sdsReserve( _MUT_ sds_t *s, size_t new_len );

// Concatenate at the end of sds.
void sdsCat( _MUT_ sds_t *s, const char *t );
void sdsCatSds( _MUT_ sds_t *s, const sds_t t );
void sdsCatLen( _MUT_ sds_t *s, const void *t, size_t len );
void sdsCatVprintf( _MUT_ sds_t *s, const char *fmt, va_list ap );
void sdsCatPrintf( _MUT_ sds_t *s, const char *fmt, ... );

// copy t to sds (overwriting existing content).
void sdsCpyLen( _MUT_ sds_t *s, const char *t, size_t len );
void sdsCpy( _MUT_ sds_t *s, const char *t );

int sdsCmp( const sds_t s1, const sds_t s2 );

// === --- Private Static Inline Fns --------------------------------------- ===

#define SDS_HEAD( s ) \
        ( (struct sds_head *)( ( s ) - ( sizeof( struct sds_head ) ) ) )
static inline size_t
_sdsLen( const sds_t s )
{
        return s == NULL ? 0 : SDS_HEAD( s )->len;
}
static inline size_t
_sdsCap( const sds_t s )
{
        return s == NULL ? 0 : SDS_HEAD( s )->alloc;
}
static inline size_t
_sdsAvail( const sds_t s )
{
        if ( s == NULL ) return 0;
        struct sds_head *p = SDS_HEAD( s );
        return p->alloc - p->len;
}
static inline void
_sdsSetLen( const sds_t s, size_t newlen )
{
        assert( s != NULL );
        SDS_HEAD( s )->len = newlen;
}
static inline void
_sdsIncLen( const sds_t s, size_t inc )
{
        assert( s != NULL );
        SDS_HEAD( s )->len += inc;
}
static inline void
_sdsSetCap( const sds_t s, size_t newcap )
{
        assert( s != NULL );
        SDS_HEAD( s )->alloc = newcap;
}
static inline void
_sdsClear( sds_t s )
{
        assert( s != NULL );
        sdsSetLen( s, 0 );
        s[0] = '\0';
}

#endif  // ADT_SDS_H_
