/* par.c — forge_par_map implementation (pthreads) */

#include "par.h"

#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

// === --- worker -------------------------------------------------------------
// ===
//

struct worker {
        size_t      start, end;
        void ( *fn )( size_t, void *, const void * );
        void       *dst_out;
        size_t      dst_stride;
        const void *ctx;
};

static void *worker_run( void *arg )
{
        struct worker *w = arg;
        for ( size_t i = w->start; i < w->end; i++ )
                w->fn( i, (char *)w->dst_out + i * w->dst_stride, w->ctx );
        return NULL;
}

// === --- forge_par_map ------------------------------------------------------
// ===
//

int forge_par_map( size_t n,
                   void ( *fn )( size_t i, void *slot_out, const void *ctx ),
                   void *dst_out, size_t dst_stride,
                   const void *ctx,
                   size_t n_threads,
                   struct err_stack *stk )
{
        if ( n == 0 )
                return 0;

        if ( n_threads == 0 ) {
                long nc   = sysconf( _SC_NPROCESSORS_ONLN );
                n_threads = ( nc > 0 ) ? (size_t)nc : 1;
        }
        if ( n_threads > n )
                n_threads = n;

        int            rc        = 0;
        size_t         n_started = 0;
        pthread_t     *threads   = NULL;
        struct worker *workers   = NULL;

        threads = malloc( n_threads * sizeof( pthread_t ) );
        workers = malloc( n_threads * sizeof( struct worker ) );
        if ( !threads || !workers ) {
                forge_err_emit( stk, "forge_par_map: malloc failed" );
                rc = 1;
                goto exit;
        }

        size_t chunk = n / n_threads;
        for ( size_t t = 0; t < n_threads; t++ ) {
                workers[t].start      = t * chunk;
                workers[t].end        = ( t + 1 == n_threads ) ? n : ( t + 1 ) * chunk;
                workers[t].fn         = fn;
                workers[t].dst_out    = dst_out;
                workers[t].dst_stride = dst_stride;
                workers[t].ctx        = ctx;
                if ( pthread_create( &threads[t], NULL, worker_run, &workers[t] ) != 0 ) {
                        forge_err_emit( stk, "forge_par_map: pthread_create failed" );
                        rc = 1;
                        goto exit;
                }
                n_started++;
        }

exit:
        for ( size_t j = 0; j < n_started; j++ )
                pthread_join( threads[j], NULL );
        free( threads );
        free( workers );
        return rc;
}
