/* error.h — forge_ error-stack infrastructure */

#ifndef ERROR_H
#define ERROR_H

#include <stddef.h>

// === --- err_stack ----------------------------------------------------------
// ===
//

struct err_stack {
        char  *buf;
        size_t len;
        size_t cap;
};

void        forge_err_init( struct err_stack *stk );
void        forge_err_deinit( struct err_stack *stk );
void        forge_err_emit( struct err_stack *stk, const char *fmt, ... );
const char *forge_err_get( const struct err_stack *stk );

#endif /* ERROR_H */
