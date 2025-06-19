#include "util.h"

#include <stdlib.h>  // NULL
#include <sys/time.h>

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
