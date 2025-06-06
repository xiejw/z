#ifndef MLM_IO_H_
#define MLM_IO_H_

#include <stdlib.h>

#include <adt/types.h>

// === --- IO Utils -------------------------------------------------------- ===

// Forward declaration.
struct io_reader;

error_t io_reader_open( const char *name, _OUT_ struct io_reader **out );
void    io_reader_close( struct io_reader * );

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
error_t io_reader_nextline( struct io_reader *, _OUT_ char **buf,
                            _OUT_ size_t *size, _OUT_ _NULLABLE_ int *partial );
#endif /* MLM_IO_H_ */
