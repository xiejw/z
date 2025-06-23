#ifndef AIC_UTIL_H_
#define AIC_UTIL_H_

#include <stdlib.h>

#include <adt/types.h>
#include <adt/vec.h>

#include "ctx.h"

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

// === --- IO Utils -------------------------------------------------------- ===

// Forward declaration.
struct io_reader;

ADT_NO_DISCARD error_t io_reader_open( struct ctx *, const char *name,
                                       _OUT_ struct io_reader **out );
void                   io_reader_close( struct io_reader * );

/* Read one line from the underlying file and point to the buffer via buf along
 * with the size in the buf.
 *
 * NOTE:
 * - The buffer is valid until close or next nextline call.
 * - NULL terminator is not placed.
 * - The '\n' is not included.
 *
 * Result code
 * - Returns OK if a line has been filed.
 * - Returns EEOF if end of file.
 * - Returns other errors if any error happends.
 *
 * If partial is not NULL, 1 means the line is partial line, 0 means it is a
 * full line.
 */
ADT_NO_DISCARD error_t io_reader_nextline( struct io_reader *, _OUT_ char **buf,
                                           _OUT_ size_t         *size,
                                           _OUT_ _NULLABLE_ int *partial );

#endif /* AIC_UTIL_H_ */
