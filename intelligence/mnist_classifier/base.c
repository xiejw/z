/* base.c — forge_ platform utilities */

#include "base.h"

#ifdef __APPLE__
#include <sys/sysctl.h>
#else
#include <unistd.h>
#endif

size_t forge_n_logical_cpus( void )
{
#ifdef __APPLE__
        int    n  = 1;
        size_t sz = sizeof( n );
        sysctlbyname( "hw.logicalcpu", &n, &sz, NULL, 0 );
        return ( n > 0 ) ? (size_t)n : 1;
#else
        long n = sysconf( _SC_NPROCESSORS_ONLN );
        return ( n > 0 ) ? (size_t)n : 1;
#endif
}
