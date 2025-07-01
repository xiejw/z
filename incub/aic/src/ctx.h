#ifndef AIC_CTX_H_
#define AIC_CTX_H_

#include <adt/types.h>

// === --- APIs ------------------------------------------------------------ ===

struct ctx;

struct ctx *ctx_new( void );
void        ctx_free( struct ctx *p );

// === --- Note Emitting APIs ---------------------------------------------- ===

/* Emit an error diagnosis note to internal buffer.
 *
 * Use ctx_dump_err_note to retrieve it.
 */
#define EMIT_ERROR_NOTE( ctx, ... )                                 \
        ctx_emit_note( ( ctx ), CTX_NOTE_ERROR, __FILE__, __LINE__, \
                       __VA_ARGS__ )

#define DUMP_ERROR_NOTE( ctx ) ctx_dump_error_note( ctx )

const char *ctx_get_error_note( struct ctx *p );
void        ctx_dump_error_note( struct ctx *p );

// === --- Logging APIs ---------------------------------------------------- ===
#define LOG_DEBUG( ctx, ... ) \
        ctx_emit_log( ctx, CTX_LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__ )
#define LOG_INFO( ctx, ... ) \
        ctx_emit_log( ctx, CTX_LOG_INFO, __FILE__, __LINE__, __VA_ARGS__ )
#define LOG_ERROR( ctx, ... ) \
        ctx_emit_log( ctx, CTX_LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__ )

// === --- Low Level APIs -------------------------------------------------- ===
enum ctx_note_level { CTX_NOTE_ERROR };

void ctx_emit_note( struct ctx *p, enum ctx_note_level level, const char *file,
                    int line, const char *fmt, ... );

enum ctx_log_level { CTX_LOG_DEBUG, CTX_LOG_INFO, CTX_LOG_ERROR };
void ctx_emit_log( struct ctx *p, enum ctx_log_level level, const char *file,
                   int line, const char *fmt, ... );
#endif /* AIC_CTX_H_ */
