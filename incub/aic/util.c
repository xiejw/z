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
