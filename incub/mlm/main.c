#include <assert.h>
#include <fcntl.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "base64.h"
#include "io.h"

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
        if ( rank_id >= 127990 ) {
                printf( "%d: ", rank_id );  // e994a6
                for ( int i = 0; piece[i] != '\0'; i++ ) {
                        printf( "%02x", (unsigned char)piece[i] );
                }
                printf( "\n" );
        }
}

/* Read tokenizer model file and create a tokenizer after that.
 *
 * Tokenizer model file consists of lines of merge-able bytes with rank.
 */
void
tok_load( Tokenizer *p )
{
        struct io_reader *r;
        error_t           err = io_reader_open( TOK_FILE, &r );

        char  *buf;
        size_t size;
        while ( ( err = io_reader_nextline( r, &buf, &size, NULL ) ) == OK ) {
                tok_process_line( p, buf, size );
        }
        io_reader_close( r );

        assert( p->pieces[TOK_MERGE_PIECE_COUNT - 1].piece != NULL );
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
