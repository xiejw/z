#include "progress.h"

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

// === --- Constants --------------------------------------------------------
// ===
//

#define MA_SIZE 16

// === --- Types ------------------------------------------------------------
// ===
//

struct sample {
        double  t;
        int64_t count;
};

// === --- Static state -----------------------------------------------------
// ===
//

static int64_t         s_total;
static char            s_title[PROGRESS_TITLE_MAX + 1];
static struct timespec s_start;
static int64_t         s_current;

static struct sample s_samples[MA_SIZE];
static int           s_sample_head;
static int           s_sample_count;

static pthread_t       s_thread;
static pthread_mutex_t s_mutex;
static volatile int    s_running;

// === --- Helpers ----------------------------------------------------------
// ===
//

static int
term_width( void )
{
        struct winsize ws;
        if ( ioctl( STDOUT_FILENO, TIOCGWINSZ, &ws ) == 0 && ws.ws_col > 0 )
                return ws.ws_col;
        return 80;
}

static double
elapsed_s( void )
{
        struct timespec now;
        clock_gettime( CLOCK_MONOTONIC, &now );
        return (double)( now.tv_sec - s_start.tv_sec ) +
               (double)( now.tv_nsec - s_start.tv_nsec ) * 1e-9;
}

/* Format ETA into buf: "Xs", "Xm", or "Xh". */
static void
fmt_eta( double s, char *buf, int len )
{
        if ( s < 60.0 )
                snprintf( buf, len, "%.0fs", s );
        else if ( s < 3600.0 )
                snprintf( buf, len, "%.0fm", s / 60.0 );
        else
                snprintf( buf, len, "%.0fh", s / 3600.0 );
}

/* Render the bar at s_current. Must be called with s_mutex held (or before
   the thread starts / after it stops). */
static void
render_bar( void )
{
        int    width   = term_width( );
        double elapsed = elapsed_s( );
        double pct =
            ( s_total > 0 ) ? (double)s_current / (double)s_total : 0.0;
        if ( pct > 1.0 ) pct = 1.0;

        /* Compute ETA */
        double eta = -1.0;
        if ( s_current > 0 && pct < 1.0 ) {
                if ( s_sample_count >= 2 ) {
                        int oldest_idx =
                            ( s_sample_head - s_sample_count + MA_SIZE ) %
                            MA_SIZE;
                        int newest_idx =
                            ( s_sample_head - 1 + MA_SIZE ) % MA_SIZE;
                        double dt =
                            s_samples[newest_idx].t - s_samples[oldest_idx].t;
                        int64_t dc = s_samples[newest_idx].count -
                                     s_samples[oldest_idx].count;
                        if ( dt > 0.0 && dc > 0 ) {
                                double rate = (double)dc / dt;
                                eta = (double)( s_total - s_current ) / rate;
                        }
                } else {
                        eta = elapsed * ( 1.0 - pct ) / pct;
                }
        }

        /* Build right-side string */
        char right[32];
        if ( eta >= 0.0 ) {
                char eta_buf[16];
                fmt_eta( eta, eta_buf, sizeof( eta_buf ) );
                snprintf( right, sizeof( right ), "ETA: %s  %.0f%%", eta_buf,
                          pct * 100.0 );
        } else {
                snprintf( right, sizeof( right ), "%.0f%%", pct * 100.0 );
        }

        /* Bar width: total - title - " [" - "] " - right */
        int title_len = (int)strlen( s_title );
        int right_len = (int)strlen( right );
        int bar_width = width - title_len - 1 - 2 - 1 - right_len;
        if ( bar_width < 4 ) bar_width = 4;

        int filled = (int)( pct * bar_width );
        if ( filled > bar_width ) filled = bar_width;

        /* Render: \r + erase line */
        fprintf( stdout, "\r\033[2K" );

        /* Title */
        fprintf( stdout, "%s [", s_title );

        /* Filled portion — green */
        fprintf( stdout, "\033[32m" );
        for ( int i = 0; i < filled; i++ )
                fputs( "\xe2\x96\x88", stdout ); /* UTF-8: █ */
        fprintf( stdout, "\033[0m" );

        /* Empty portion — dim */
        fprintf( stdout, "\033[2m" );
        for ( int i = filled; i < bar_width; i++ )
                fputs( "\xe2\x96\x91", stdout ); /* UTF-8: ░ */
        fprintf( stdout, "\033[0m" );

        fprintf( stdout, "] " );

        /* ETA (yellow) + percentage (cyan) */
        if ( eta >= 0.0 ) {
                char eta_buf[16];
                fmt_eta( eta, eta_buf, sizeof( eta_buf ) );
                fprintf( stdout,
                         "\033[33mETA: %s\033[0m  \033[36m%.0f%%\033[0m",
                         eta_buf, pct * 100.0 );
        } else {
                fprintf( stdout, "\033[36m%.0f%%\033[0m", pct * 100.0 );
        }

        fflush( stdout );
}

// === --- Background thread ------------------------------------------------
// ===
//

static void *
progress_thread( void *arg )
{
        (void)arg;
        struct timespec ts = { 0, 200 * 1000 * 1000 }; /* 200 ms */
        while ( s_running ) {
                nanosleep( &ts, NULL );
                if ( !s_running ) break;
                pthread_mutex_lock( &s_mutex );
                render_bar( );
                pthread_mutex_unlock( &s_mutex );
        }
        return NULL;
}

// === --- Public API -------------------------------------------------------
// ===
//

void
forge_progress_bar_open( int64_t total, const char *title )
{
        s_total        = total;
        s_current      = 0;
        s_sample_head  = 0;
        s_sample_count = 0;
        snprintf( s_title, sizeof( s_title ), "%s", title );
        clock_gettime( CLOCK_MONOTONIC, &s_start );

        pthread_mutex_init( &s_mutex, NULL );
        s_running = 1;
        pthread_create( &s_thread, NULL, progress_thread, NULL );
}

void
forge_progress_bar_advance( int64_t current )
{
        pthread_mutex_lock( &s_mutex );
        s_current                = current;
        s_samples[s_sample_head] = (struct sample){ elapsed_s( ), current };
        s_sample_head            = ( s_sample_head + 1 ) % MA_SIZE;
        if ( s_sample_count < MA_SIZE ) s_sample_count++;
        render_bar( );
        pthread_mutex_unlock( &s_mutex );
}

void
forge_progress_bar_close( void )
{
        s_running = 0;
        pthread_join( s_thread, NULL );
        pthread_mutex_destroy( &s_mutex );
        render_bar( ); /* finalize at s_current, not s_total */
        fputc( '\n', stdout );
        fflush( stdout );
}
