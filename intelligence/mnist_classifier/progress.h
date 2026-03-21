#pragma once

#include <stdint.h>

/* Maximum characters of the title displayed on the left.
 * Override at compile time with -DPROGRESS_TITLE_MAX=N. */
#ifndef PROGRESS_TITLE_MAX
#define PROGRESS_TITLE_MAX 20
#endif

/* Open a progress bar. total is the final count; title is shown on the left. */
void forge_progress_bar_open( int64_t total, const char *title );

/* Redraw the bar at position current (absolute, 0 <= current <= total). */
void forge_progress_bar_advance( int64_t current );

/* Finalize the bar at 100% and move to the next line. */
void forge_progress_bar_close( void );
