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

#define TOK_READ_BUF_SIZE     4096
#define TOK_MAX_ID_LEN        10
#define TOK_MERGE_PIECE_COUNT 128000

#define PANIC( msg )                 \
        do {                         \
                printf( "panic\n" ); \
                printf( msg );       \
                exit( -1 );          \
        } while ( 0 )

#define DEBUG_PRINT 0

#define DEBUG \
        if ( DEBUG_PRINT ) printf

/* === Data Structure --------------------------------------------------------
 */
typedef struct {
        char *piece;
        int   id;
} MergePiece;

typedef struct {
        MergePiece pieces[TOK_MERGE_PIECE_COUNT];
} Tokenizer;

/* === State Machine to split text into words ----------------------------------
 *
 * The entire world is "copying" OpenAI's tiktoken. It uses a quite complex
 * regexp to split text into words before applying bpe.
 *
 * Instead of using a fancy dependency, e.g, pcre2, here I wrote a state
 * machine manually. The downside is unicode support becomes a feature request
 * later to understand \p{L} and \p{N}; in addition to correctly parse unicode
 * byte stream.
 */

int
is_chr_letter( char c )
{
        return ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' );
}

int
is_chr_digit( char c )
{
        return ( c >= '0' && c <= '9' );
}

char
convert_to_lower( char c )
{
        if ( c >= 'A' && c <= 'Z' ) {
                return c - 'A' + 'a';
        }
        return c;
}

int
is_whitespace( char c )
{
        return ( c == ' ' || c == '\t' || c == '\r' || c == '\n' );
}

int
is_newline( char c )
{
        return ( c == '\r' || c == '\n' );
}

void
tok_split_text_to_words( Tokenizer *p, const char *text,
                         /*output*/ char ***words, int *count )
{
        (void)p;
        (void)text;
        (void)words;
        (void)count;
}

void
tok_encode( Tokenizer *p, const char *text )
{
        char **words;
        int    word_count;
        tok_split_text_to_words( p, text, &words, &word_count );
}

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

Tokenizer *
tok_new( void )
{
        Tokenizer *p = calloc( 1, sizeof( *p ) );
        assert( p != NULL );
        return p;
}

void
tok_free( Tokenizer *p )
{
        if ( p == NULL ) return;
        for ( int i = 0; i < TOK_MERGE_PIECE_COUNT; i++ ) {
                free( p->pieces[i].piece );
        }
}

/* One time initialized lookup table to avoid switch cost during base64
 * decoding.
 *
 * See base54_lookup_tbl_init why 1 is here.
 */
char BASE64_LOOKUP_TBL[256] = { 1 };

/* One time initialize the base64 lookup table. */
void
base54_lookup_tbl_init( void )
{
        /* Non one means this has been initialized. */
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
 * string for the decoded result. Caller takes the ownership of the output.
 *
 * Check https://en.wikipedia.org/wiki/Base64 for reference.
 *
 * Algorithm
 * - Unpack 4 chars each time. Fill the bits into the 3 chars of the
 *   output.
 * - The '=' padding is ignored as it just becomes a NULL terminator.
 */
char *
base64_decode( char *buf, size_t len )
{
        base54_lookup_tbl_init( );
        assert( len > 0 && len % 4 == 0 );
        char *output = malloc( len / 4 * 3 + 1 );

        /* Once lookup up table is initialized, all valid chars, except '=' or
         * 'A', should have non zero value. */
#define CHECK_VALID_BASE64_CH( idx )                          \
        assert( buf[( idx )] == '=' || buf[( idx )] == 'A' || \
                BASE64_LOOKUP_TBL[(int)buf[( idx )]] != 0 )

        size_t output_idx = 0;
        for ( size_t i = 0; i < len; i += 4 ) {
                CHECK_VALID_BASE64_CH( i );
                CHECK_VALID_BASE64_CH( i + 1 );
                CHECK_VALID_BASE64_CH( i + 2 );
                CHECK_VALID_BASE64_CH( i + 3 );

                char a = BASE64_LOOKUP_TBL[(int)buf[i]];
                char b = BASE64_LOOKUP_TBL[(int)buf[i + 1]];
                char c = BASE64_LOOKUP_TBL[(int)buf[i + 2]];
                char d = BASE64_LOOKUP_TBL[(int)buf[i + 3]];

                output[output_idx++] = (char)( ( a << 2 ) + ( b >> 4 ) );
                output[output_idx++] =
                    (char)( ( ( b & 0xF ) << 4 ) + ( c >> 2 ) );
                output[output_idx++] = (char)( ( ( c & 0x4 ) << 6 ) + d );
        }

#undef CHECK_VALID_BASE64_CH

        output[output_idx] = '\0';
        return output;
}

/* Process one line read from tokenizer model file.
 *
 * This function does not assume buf ends with NULL. The length is provided as
 * len. But in case buf has trailing NULL '\0' or new line '\n', they will be
 * excluded from the processing.
 *
 * In addition, by definition of the tokenizer model file, empty line is
 * ignored as well.
 */
void
tok_process_line( Tokenizer *p, char *buf, size_t len )
{
        /* Strip off new line or final \0. */
        while ( len >= 1 && ( buf[len - 1] == '\0' || buf[len - 1] == '\n' ) )
                len--;

        /* Skip empty line */
        if ( len == 0 ) return;

        /* Split by space, and expect 2 components. */
        char *space_ptr;
        space_ptr = strnstr( buf, " ", len );
        assert( space_ptr != NULL &&
                "expect a space in tokenizer model file line." );
        assert( strnstr( space_ptr + 1, " ",
                         len - (size_t)( space_ptr - buf + 1 ) ) == NULL &&
                "expect one space only for each line in tokenizer model file" );

        /* Decode merge-able piece */
        char *piece = base64_decode( buf, (size_t)( space_ptr - buf ) );

        /* Decode rank id */
        char   id_buf[TOK_MAX_ID_LEN]; /* strtol expects NULL-terminator.*/
        size_t id_len = len - (size_t)( space_ptr + 1 - buf );
        assert( id_len < TOK_MAX_ID_LEN );
        buf = space_ptr + 1;
        memcpy( id_buf, buf, id_len );
        id_buf[id_len] = '\0';
        char *endptr;
        int   rank_id = (int)strtol( id_buf, &endptr, 10 );
        assert( endptr == id_buf + id_len );

        /* Store into Tokenizer */
        assert( rank_id >= 0 && rank_id <= TOK_MERGE_PIECE_COUNT );
        assert( p->pieces[rank_id].piece == NULL );
        /* The model file is sequentially recorded. */
        assert( rank_id == 0 || p->pieces[rank_id - 1].piece != NULL );
        p->pieces[rank_id].piece = piece;
        p->pieces[rank_id].id    = rank_id;

        DEBUG( "decode %s rank %d\n", piece, rank_id );
}

/* Read tokenizer model file and create a tokenizer after that.
 *
 * Tokenizer model file consists of lines of merge-able bytes with rank.
 */
void
tok_load( Tokenizer *p )
{
        char   buf[TOK_READ_BUF_SIZE];
        char   line[TOK_READ_BUF_SIZE];
        size_t line_idx = 0;

        /* The heavy work is reading file line by line and process them.
         *
         * For performance, we read page by page and extract lines from it.
         */
        int fd = open( TOK_FILE, O_RDONLY );
        if ( fd == -1 ) PANIC( "failed to open tok file" );

        while ( 1 ) {
                ssize_t c = read( fd, buf, TOK_READ_BUF_SIZE );
                if ( c < 0 ) PANIC( "unexpected tok file read error." );
                if ( c == 0 ) {
                        /* Final line in buffer */
                        if ( line_idx > 0 ) {
                                assert( line_idx <= TOK_READ_BUF_SIZE );
                                tok_process_line( p, line, line_idx );
                        }
                        break;
                }

                size_t start = 0;
                while ( 1 ) {
                        size_t end = start;
                        for ( ; buf[end] != '\n' && end < (size_t)c; end++ ) {
                        }
                        if ( end == (size_t)c ) {
                                assert( end - start + line_idx <=
                                        TOK_READ_BUF_SIZE );
                                memcpy( line + line_idx, buf + start,
                                        end - start );
                                line_idx += end - start;
                                break;
                        }

                        /* Found a line */
                        memcpy( line + line_idx, buf + start, end - start );
                        tok_process_line( p, line, end - start + line_idx );
                        line_idx = 0;
                        start    = end + 1;
                }
        }

        assert( p->pieces[TOK_MERGE_PIECE_COUNT - 1].piece != NULL );
        close( fd );
}

/* === Main ----------------------------------------------------------------- */

int
main( void )
{
        Tokenizer *p = tok_new( );
        tok_load( p );
        tok_encode( p, "hello world   " );
        tok_free( p );
}
