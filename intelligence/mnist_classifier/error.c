#include "error.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// === --- err_stack ----------------------------------------------------------
// ===
//

void
forge_err_init( struct err_stack *stk )
{
        stk->buf = NULL;
        stk->len = 0;
        stk->cap = 0;
}

void
forge_err_deinit( struct err_stack *stk )
{
        free( stk->buf );
        stk->buf = NULL;
        stk->len = 0;
        stk->cap = 0;
}

void
forge_err_emit( struct err_stack *stk, const char *fmt, ... )
{
        /* First call: root-cause prefix "✗ " (U+2717, UTF-8: e2 9c 97).
         * Subsequent calls: context prefix "↳ " (U+21B3, UTF-8: e2 86 b3).
         * Each call appends '\n' automatically. */
        const char *prefix =
            ( stk->len == 0 ) ? "\xe2\x9c\x97 " : "\xe2\x86\xb3 ";
        size_t prefix_len = 4; /* 3-byte UTF-8 glyph + space */

        va_list ap1, ap2;
        va_start( ap1, fmt );
        va_copy( ap2, ap1 );
        int msg_len = vsnprintf( NULL, 0, fmt, ap1 );
        va_end( ap1 );
        if ( msg_len < 0 ) {
                va_end( ap2 );
                return;
        }

        /* prefix + message + '\n' + '\0' */
        size_t needed = stk->len + prefix_len + (size_t)msg_len + 2;
        if ( needed > stk->cap ) {
                size_t new_cap = ( stk->cap == 0 ) ? 256 : stk->cap;
                while ( new_cap < needed ) new_cap *= 2;
                char *nb = realloc( stk->buf, new_cap );
                if ( !nb ) {
                        va_end( ap2 );
                        return; /* silently truncate on alloc failure */
                }
                stk->buf = nb;
                stk->cap = new_cap;
        }

        memcpy( stk->buf + stk->len, prefix, prefix_len );
        stk->len += prefix_len;

        vsnprintf( stk->buf + stk->len, (size_t)msg_len + 1, fmt, ap2 );
        va_end( ap2 );
        stk->len += (size_t)msg_len;

        stk->buf[stk->len++] = '\n';
        stk->buf[stk->len]   = '\0';
}

const char *
forge_err_get( const struct err_stack *stk )
{
        return ( stk->len == 0 ) ? NULL : stk->buf;
}
