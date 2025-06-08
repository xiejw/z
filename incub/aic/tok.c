/* === Tokenizer ----------------------------------------------------------- ===
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
#include "tok.h"

#include <assert.h>
#include <fcntl.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <adt/sds.h>

#include "base64.h"
#include "io.h"

#define TOK_MAX_ID_LEN        10
#define TOK_MERGE_PIECE_COUNT 128000

#ifndef NDEBUG
#define DEBUG_PRINT 1
#else
#define DEBUG_PRINT 0
#endif

// === Data Structure ------------------------------------------------------ ===
struct mergable_rank {
        char *mergable; /* owned. */
        int   rank;
};

struct tokenizer {
        struct ctx          *ctx;
        struct mergable_rank mergable_ranks[TOK_MERGE_PIECE_COUNT];
};

// === Private Helper Methods----------------------------------------------- ===

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
static void
tok_split_text_to_words( struct tokenizer *p, const char *text,
                         /*output*/ char ***words, int *count )
{
        (void)p;
        (void)text;
        (void)words;
        (void)count;
}

// === Tokenizer Model File and Mergable Ranks ---------------------------------

/* Return -1 if not found or 0-based index of the pos of space .*/
static int
tok_find_next_space( const char *buf, int len )
{
        int i;
        for ( i = 0; i < len && buf[i] != ' '; i++ ) {
        }
        return ( i == len ) ? -1 : i;
}

/* A static array for the mergable_ranks from 127990 to 127999. This table is
 * printed in the Python world so we could sanity check whether the logic here
 * is correct or not. */
static const int   tok_mergable_ranks_starting_id = 127990;
static const char *tok_mergable_ranks[]           = {
    /* 127990=*/"e0b98ce0b881e0b8a3",
    /* 127991 */ "ceb6ceb1",
    /* 127992 */ "20eb8d94ec9ab1",
    /* 127993 */ "d988d984d8a7d8aa",
    /* 127994 */ "d0b2d0b0d182d0b8d181d18f",
    /* 127995 */ "206bc3b66b",
    /* 127996 */ "d986d8a8",
    /* 127997 */ "20d0b2d18bd181d0bed0bad0bed0b9",
    /* 127998 */ "e383bce383bc",
    /* 127999 */ "e994a6" };

/* Process one line read from tokenizer model file.
 *
 * This function does not assume buf ends with NULL. The length is provided as
 * len. But in case buf has trailing NULL '\0' or new line '\n', they will be
 * excluded from the processing.
 *
 * In addition, by definition of the tokenizer model file, empty line is
 * ignored as well.
 */
static void
tok_process_one_line_of_model_file( struct tokenizer *p, char *buf, size_t len )
{
        /* Strip off new line or final \0. */
        while ( len >= 1 && ( buf[len - 1] == '\0' || buf[len - 1] == '\n' ) )
                len--;

        /* Skip empty line */
        if ( len == 0 ) return;

        /* Split by space, and expect 2 components. */
        int space_id;
        space_id = tok_find_next_space( buf, (int)len );
        assert( space_id != -1 &&
                "expect a space in tokenizer model file line." );
        assert( tok_find_next_space( buf + space_id + 1,
                                     (int)len - ( space_id + 1 ) ) == -1 &&
                "expect one space only for each line in tokenizer model file" );

        /* Decode merge-able piece */
        char *mergable = base64_decode( buf, (size_t)space_id );

        /* Decode rank id */
        char   id_buf[TOK_MAX_ID_LEN]; /* strtol expects NULL-terminator.*/
        size_t id_len = len - (size_t)( space_id + 1 );
        assert( id_len < TOK_MAX_ID_LEN );
        buf += space_id + 1;
        memcpy( id_buf, buf, id_len );
        id_buf[id_len] = '\0';
        char *endptr;
        int   rank_id = (int)strtol( id_buf, &endptr, 10 );
        assert( endptr == id_buf + id_len );

        /* Store into Tokenizer */
        assert( rank_id >= 0 && rank_id <= TOK_MERGE_PIECE_COUNT );
        assert( p->mergable_ranks[rank_id].mergable == NULL );
        /* The model file is sequentially recorded. */
        assert( rank_id == 0 ||
                p->mergable_ranks[rank_id - 1].mergable != NULL );
        p->mergable_ranks[rank_id].mergable = mergable;
        p->mergable_ranks[rank_id].rank     = rank_id;

#ifndef NDBUG
        /* Perform sanity check. See tok_mergable_ranks for details. */
        if ( rank_id >= tok_mergable_ranks_starting_id ) {
                sds_t s = sds_empty_with_cap( 200 );
                int   i;
                for ( i = 0; mergable[i] != '\0'; i++ ) {
                        sds_cat_printf( &s, "%02x",
                                        (unsigned char)mergable[i] );
                }
                const char *expected_piece =
                    tok_mergable_ranks[rank_id -
                                       tok_mergable_ranks_starting_id];
                // DEBUG( "expect %s got %s\n", expected_piece, s );
                assert( 0 == strcmp( expected_piece, s ) );
                sds_free( s );
        }
#endif
}

static error_t
tok_load_model_file( struct ctx *ctx, struct tokenizer *p, const char *fname )
{
        struct io_reader *r   = NULL;
        error_t           err = io_reader_open( ctx, fname, &r );
        if ( err != OK ) {
                EMIT_ERROR_NOTE( ctx, "failed to open tokenizer model file." );
                return err;
        }

        char  *buf;
        size_t size;
        while ( ( err = io_reader_nextline( r, &buf, &size, NULL ) ) == OK ) {
                tok_process_one_line_of_model_file( p, buf, size );
        }
        if ( err != EEOF ) {
                goto cleanup;
        }
        err = OK;
        assert( p->mergable_ranks[TOK_MERGE_PIECE_COUNT - 1].mergable != NULL );
        if ( DEBUG_PRINT )
                LOG_DEBUG( ctx, "Passed. tok_mergable_ranks sanity check." );

cleanup:
        io_reader_close( r );
        return err;
}

// === Implementation ------------------------------------------------------ ===
//
error_t
tok_new( struct ctx *ctx, const char *tok_model_name, struct tokenizer **pp )
{
        struct tokenizer *p = calloc( 1, sizeof( *p ) );
        assert( p != NULL );
        error_t err = tok_load_model_file( ctx, p, tok_model_name );
        if ( err != OK ) return err;
        p->ctx = ctx;
        *pp    = p;
        return OK;
}

void
tok_free( struct tokenizer *p )
{
        if ( p == NULL ) return;
        for ( int i = 0; i < TOK_MERGE_PIECE_COUNT; i++ ) {
                free( p->mergable_ranks[i].mergable );
        }
        free( p );
}

void
tok_encode( struct tokenizer *p, const char *text, vec_t( int ) * ps )
{
        char **words;
        int    word_count;
        tok_split_text_to_words( p, text, &words, &word_count );
        (void)ps;
}
