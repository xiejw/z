#include <assert.h>
#include <fcntl.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* === Configuration -------------------------------------------------------- */
#ifndef TOK_FILE
#error TOK_FILE should be passed via "-DTOK_FILE"
#endif

#define TOK_READ_BUF_SIZE 4096

#define PANIC( msg )                 \
        do {                         \
                printf( "panic\n" ); \
                printf( msg );       \
                exit( -1 );          \
        } while ( 0 )

/* === Tokenizer ---------------------------------------------------------------
 *
 * There are few data structures needed
 * - To identify all special tokens, either a trie or regex is used.
 * - To identify word boundary, seems regex is the best tool but c does not
 *   have good support (unless using a heavy external library). Alternatively,
 *   write my own state machine (this is how llama.cpp does which aligns my
 *   guess)
 * - For mergeable ranks, we either need a hash table or trie to speed up the
 *   lookup.  It is used very frequently so performance is important.
 *
 * File to read
 * - Load tok file
 *   https://github.com/openai/tiktoken/blob/00813b3f987a083ee9f631620d0271b0169da58b/tiktoken/load.py#L146-L158
 */

char BASE64_ARRAY[256] = { 1 }; /* See base64_arary_init why 1 is here. */

void
base64_arary_init( void )
{
        if ( BASE64_ARRAY[0] != 1 ) return;

        char idx = 0;
        for ( char i = 'A'; i <= 'Z'; i++ ) {
                BASE64_ARRAY[(int)i] = idx++;
        }
        assert( idx == 26 );

        for ( char i = 'a'; i <= 'z'; i++ ) {
                BASE64_ARRAY[(int)i] = idx++;
        }
        assert( idx == 26 + 26 );

        for ( char i = '0'; i <= '9'; i++ ) {
                BASE64_ARRAY[(int)i] = idx++;
        }
        assert( idx == 26 + 26 + 10 );

        BASE64_ARRAY[(int)'+'] = idx++;
        BASE64_ARRAY[(int)'/'] = idx++;
        assert( idx == 64 );

        BASE64_ARRAY[(int)'='] = 0;

        BASE64_ARRAY[0] = 0; /* Record init is done. */
}
char *
base64_decode( char *buf, size_t len )
{
        base64_arary_init( );
        assert( len > 0 && len % 4 == 0 );
        /* Algorithm
         * - Unpack 4 chars each time. Fill the bits into the 3 chars of the
         *   output.
         * - Identify the '=' padding and reduce the length of the output.
         */
        (void)buf;
        (void)len;
        return NULL;
}

/* terminate 0 and '\n' are excluded. */
void
process_line_in_tok_file( char *buf, size_t len )
{
        while ( len >= 1 && ( buf[len - 1] == '\0' || buf[len - 1] == '\n' ) )
                len--;
        if ( len == 0 ) return; /* Skip empty line */
        printf( "%.*s\n", (int)len, buf );
}

/* Read tokenizer model file and create a tokenizer after that.
 *
 * Tokenizer model file is lines of merge-able bytes with rank.
 */
void
read_tok_file( void )
{
        char   buf[TOK_READ_BUF_SIZE];
        char   line[TOK_READ_BUF_SIZE];
        size_t line_idx = 0;

        int fd = open( TOK_FILE, O_RDONLY );
        if ( fd == -1 ) PANIC( "failed to open tok file" );

        while ( 1 ) {
                ssize_t c = read( fd, buf, TOK_READ_BUF_SIZE - line_idx );
                if ( c < 0 ) PANIC( "unexpected tok file read error." );
                if ( c == 0 ) {
                        /* Final line in buffer */
                        if ( line_idx > 0 )
                                process_line_in_tok_file( line, line_idx );
                        break;
                }

                size_t start = 0;
                while ( 1 ) {
                        size_t end = start;
                        for ( ; buf[end] != '\n' && end < (size_t)c; end++ ) {
                        }
                        if ( end == (size_t)c ) {
                                memcpy( line + line_idx, buf + start,
                                        end - start );
                                line_idx += end - start;
                                break;
                        }

                        /* Found a line */
                        memcpy( line + line_idx, buf + start, end - start );
                        process_line_in_tok_file( line,
                                                  end - start + line_idx );
                        line_idx = 0;
                        start    = end + 1;
                }
        }

        close( fd );
}

/* === Main ----------------------------------------------------------------- */

int
main( void )
{
        read_tok_file( );
}
