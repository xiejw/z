#ifndef MLM_BASE64_H_
#define MLM_BASE64_H_

#include <stdlib.h>

/* Decode the input string buf with length len and return a malloc-allocated
 * string for the decoded result. Caller takes the ownership of the output.
 *
 * Check https://en.wikipedia.org/wiki/Base64 for reference.
 *
 * Algorithm
 * - Unpack 4 chars each time. Fill the bits into the 3 chars of the
 *   output.
 * - The '=' padding is ignored as it just becomes a NULL terminator.
 */
char *base64_decode( char *buf, size_t len );

#endif /* MLM_BASE64_H_ */
