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
        void       *out;
        size_t      out_stride;
        const void *ctx;
};

static void *worker_run( void *arg )
{
        struct worker *w = arg;
        for ( size_t i = w->start; i < w->end; i++ )
                w->fn( i, (char *)w->out + i * w->out_stride, w->ctx );
        return NULL;
}

// === --- forge_par_map ------------------------------------------------------
// ===
//

int forge_par_map( size_t n,
                   void ( *fn )( size_t i, void *out_slot, const void *ctx ),
                   void *out, size_t out_stride,
                   const void *ctx,
                   size_t n_threads,
                   struct err_stack *stk )
{
        if ( n == 0 )
                return 0;

        if ( n_threads == 0 ) {
                long nc  = sysconf( _SC_NPROCESSORS_ONLN );
                n_threads = ( nc > 0 ) ? (size_t)nc : 1;
        }
        if ( n_threads > n )
                n_threads = n;

        pthread_t    *threads = malloc( n_threads * sizeof( pthread_t ) );
        struct worker *workers = malloc( n_threads * sizeof( struct worker ) );
        if ( !threads || !workers ) {
                free( threads );
                free( workers );
                forge_err_emit( stk, "forge_par_map: malloc failed" );
                return 1;
        }

        size_t chunk = n / n_threads;
        for ( size_t t = 0; t < n_threads; t++ ) {
                workers[t].start      = t * chunk;
                workers[t].end        = ( t + 1 == n_threads ) ? n : ( t + 1 ) * chunk;
                workers[t].fn         = fn;
                workers[t].out        = out;
                workers[t].out_stride = out_stride;
                workers[t].ctx        = ctx;
                if ( pthread_create( &threads[t], NULL, worker_run, &workers[t] ) != 0 ) {
                        forge_err_emit( stk, "forge_par_map: pthread_create failed" );
                        for ( size_t j = 0; j < t; j++ )
                                pthread_join( threads[j], NULL );
                        free( threads );
                        free( workers );
                        return 1;
                }
        }

        for ( size_t t = 0; t < n_threads; t++ )
                pthread_join( threads[t], NULL );

        free( threads );
        free( workers );
        return 0;
}
