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
#define TOK_MAX_ID_LEN    10

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

char BASE64_LOOKUP_TBL[256] = {
    1 }; /* See base54_lookup_tbl_init why 1 is here. */

/* One time initialize the base64 lookup table. */
void
base54_lookup_tbl_init( void )
{
        if ( BASE64_LOOKUP_TBL[0] != 1 ) return;

        char idx = 0;
        for ( char i = 'A'; i <= 'Z'; i++ ) {
                BASE64_LOOKUP_TBL[(int)i] = idx++;
        }
        assert( idx == 26 );

        for ( char i = 'a'; i <= 'z'; i++ ) {
                BASE64_LOOKUP_TBL[(int)i] = idx++;
        }
        assert( idx == 26 + 26 );

        for ( char i = '0'; i <= '9'; i++ ) {
                BASE64_LOOKUP_TBL[(int)i] = idx++;
        }
        assert( idx == 26 + 26 + 10 );

        BASE64_LOOKUP_TBL[(int)'+'] = idx++;
        BASE64_LOOKUP_TBL[(int)'/'] = idx++;
        assert( idx == 64 );

        BASE64_LOOKUP_TBL[(int)'='] = 0;

        BASE64_LOOKUP_TBL[0] = 0; /* Record init is done. */
}

/* Decode the input string buf with length len and return a malloc-allocated
 * string for the decoded result.
 *
 * Check https://en.wikipedia.org/wiki/Base64 for reference.
 */
char *
base64_decode( char *buf, size_t len )
{
        base54_lookup_tbl_init( );

        assert( len > 0 && len % 4 == 0 );
        /* Algorithm
         * - Unpack 4 chars each time. Fill the bits into the 3 chars of the
         *   output.
         * - The '=' padding is ignored as it just becomes a NULL terminator.
         */

        char *output = malloc( len / 4 * 3 + 1 );

#define CHECK_VALID_BASE64_CH( idx )                          \
        assert( buf[( idx )] == '=' || buf[( idx )] == 'A' || \
                BASE64_LOOKUP_TBL[(int)buf[( idx )]] != 0 )

        size_t output_idx = 0;
        for ( size_t i = 0; i < len; i += 4 ) {
                CHECK_VALID_BASE64_CH( i );
                CHECK_VALID_BASE64_CH( i + 1 );
                CHECK_VALID_BASE64_CH( i + 2 );
                CHECK_VALID_BASE64_CH( i + 3 );
                char a               = BASE64_LOOKUP_TBL[(int)buf[i]];
                char b               = BASE64_LOOKUP_TBL[(int)buf[i + 1]];
                char c               = BASE64_LOOKUP_TBL[(int)buf[i + 2]];
                char d               = BASE64_LOOKUP_TBL[(int)buf[i + 3]];
                output[output_idx++] = (char)( ( a << 2 ) + ( b >> 4 ) );
                output[output_idx++] =
                    (char)( ( ( b & 0xF ) << 4 ) + ( c >> 2 ) );
                output[output_idx++] = (char)( ( ( c & 0x4 ) << 6 ) + d );
        }

#undef CHECK_VALID_BASE64_CH

        output[output_idx] = '\0';
        return output;
}

/* terminate 0 and '\n' are excluded. buf is allowed to not null-terminated */
void
process_line_in_tok_file( char *buf, size_t len )
{
        char id_buf[TOK_MAX_ID_LEN];
        /* Strip off new line or final \0. */
        while ( len >= 1 && ( buf[len - 1] == '\0' || buf[len - 1] == '\n' ) )
                len--;

        /* Skip empty line */
        if ( len == 0 ) return;

        /* Split by space */
        char *space_ptr;
        space_ptr = strnstr( buf, " ", len );
        if ( space_ptr == NULL )
                PANIC( "expect a space in tokenizer model file line." );

        assert( strnstr( space_ptr + 1, " ",
                         len - (size_t)( space_ptr - buf + 1 ) ) == NULL &&
                "expect one space only" );

        /* Decode mergeable piece */
        char *output = base64_decode( buf, (size_t)( space_ptr - buf ) );

        /* Decode rank id */
        size_t id_len = len - (size_t)( space_ptr + 1 - buf );
        assert( id_len < TOK_MAX_ID_LEN );
        memcpy( id_buf, space_ptr + 1, id_len );
        id_buf[id_len] = '\0';
        char *endptr;
        int   rank_id = (int)strtol( id_buf, &endptr, 10 );
        assert( endptr == id_buf + id_len );

        printf( "decode %s rank %d\n", output, rank_id );
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
