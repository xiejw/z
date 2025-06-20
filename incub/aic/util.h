#ifndef AIC_UTIL_H_
#define AIC_UTIL_H_

#include <adt/types.h>
#include <adt/vec.h>

// === --- Time ------------------------------------------------------------ ===

/* Returns milli seconds since epoch as double. */
double time_ms_since_epoch( void );

// === --- Encoding -------------------------------------------------------- ===

// ------- Byte Code------------------------------------------------------------

/* Store and load u64 into (from) bytecode. */
void bytecode_store_u64( vec_t( byte ) * pcode, u64 v );
u64  bytecode_load_u64( byte *ptr );

// ------- Base64 --------------------------------------------------------------

/* Decode the input string buf with length len and return a malloc-allocated
 * string for the decoded result.
 *
 * Caller takes the ownership of the output.
 */
char *base64_decode( char *buf, size_t len );

#endif /* AIC_UTIL_H_ */
