/* === Tokenizer ----------------------------------------------------------- ===
 *
 * There are few data structures needed
 * - To identify all special tokens, either a trie or regex is used.
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

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

#include <adt/sds.h>
#include <adt/vec.h>

#include "base64.h"
#include "io.h"
#include "util.h"

#define TOK_MAX_ID_LEN              10
#define TOK_MERGEALBE_RANK_COUNT    128000
#define TOK_MERGEABLE_HASH_TBL_SIZE 131072 /* The next 2 to power int. */

#define TOK_RESERVED_SPECIAL_TOKENS_COUNT         256
#define TOK_RESERVED_SPECIAL_TOKENS_HASH_TBL_SIZE 512

#define TOK_SPLIT_PAT                                                          \
        "(?i:'s|'t|'re|'ve|'m|'ll|'d)|[^\\r\\n\\p{L}\\p{N}]?\\p{L}+|\\p{N}{1," \
        "3}| ?[^\\s\\p{L}\\p{N}]+[\\r\\n]*|\\s*[\\r\\n]+|\\s+(?!\\S)|\\s+"

#define TOK_LOGGING_PREFIX "[Tok] "

#ifndef NDEBUG
#define DEBUG_PRINT 0
#else
#define DEBUG_PRINT 0
#endif

// === Data Structure ------------------------------------------------------ ===
struct mergeable_rank {
        char *mergeable; /* Owned. */
        int   rank;
};

struct tokenizer {
        struct ctx *ctx; /* Unowned. */

        /* Used for splitting words. */
        pcre2_code       *re;
        pcre2_match_data *match_data;

        /* Used for bpe + mergeable_ranks. */
        struct mergeable_rank mergeable_ranks[TOK_MERGEALBE_RANK_COUNT];
        vec_t( struct mergeable_rank * ) hashes[TOK_MERGEABLE_HASH_TBL_SIZE];

        /* Used for special tokens. */
        struct mergeable_rank special_tokens[TOK_RESERVED_SPECIAL_TOKENS_COUNT];
        vec_t( struct mergeable_rank * )
            special_tokens_hashes[TOK_RESERVED_SPECIAL_TOKENS_HASH_TBL_SIZE];

        /* Special ids for fast access */
        int id_begin_of_text;
        int id_end_of_turn;
        int id_start_of_header;
        int id_end_of_header;
};

// === --- Private Helper Methods ------------------------------------------ ===

// === Simple Hash Table -------------------------------------------------------
//
// The design of the hash table is to be super fast.  In particular,
// - No input is assumed to be NULL-terminator. This saves a lot of unnecessary
//   copies, especially in bpe code.
// - Super simple hash fn to save compute.
//
// The mergeable ranks are known at ahead of time. And this is an internal
// application which is not subject to hash flood attaching. So, the hash
// function should be really fast and simple.
//
// We measure the length of the chains which has maximum at 8. So conflicts are
// OK.

static size_t
hash_fn( const char *text, size_t len )
{
        size_t h = 0;
        for ( size_t i = 0; i < len; i++ ) {
                const char c = text[i];
                h += (size_t)c;
                h *= 37;
        }
        return h;
}

/* Build a hash table to store all mergeable_ranks.
 *
 * NOTE:
 * - This function assume hashes array are zero initialized.
 * - And tbl_size is 2 to the power.
 */
static void
hash_init( struct ctx *ctx, struct mergeable_rank *mergeable_ranks,
           size_t item_count, vec_t( struct mergeable_rank * ) * hashes,
           size_t tbl_size )
{
#ifndef NDEBUG
        size_t max_hash_chain_count = 0;
#endif
        for ( size_t i = 0; i < item_count; i++ ) {
                const char *text = mergeable_ranks[i].mergeable;
                size_t      hash = hash_fn( text, strlen( text ) );
                hash &= ( tbl_size - 1 );
                vec_push( &hashes[hash], &mergeable_ranks[i] );
#ifndef NDEBUG
                size_t vec_size = vec_size( hashes[hash] );
                if ( vec_size > max_hash_chain_count )
                        max_hash_chain_count = vec_size;
#endif
        }
#ifndef NDEBUG
        LOG_DEBUG(
            ctx, TOK_LOGGING_PREFIX "Max hash chain size for hash table is %d",
            (int)max_hash_chain_count );
#else
        (void)ctx;
#endif
}

static void
hash_deinit( vec_t( struct mergeable_rank * ) * hashes, size_t tbl_size )
{
        for ( size_t i = 0; i < tbl_size; i++ ) {
                vec_free( hashes[i] );
        }
}

// NULL if not found
static struct mergeable_rank *
hash_search( vec_t( struct mergeable_rank * ) * hashes, size_t tbl_size,
             const char *text, size_t len )
{
        size_t hash = hash_fn( text, len );
        hash &= ( tbl_size - 1 );
        vec_t( struct mergeable_rank * ) p = hashes[hash];
        size_t vec_size                    = vec_size( p );
        if ( vec_size == 0 ) return NULL;

        for ( size_t i = 0; i < vec_size; i++ ) {
                struct mergeable_rank *candidate = p[i];
                const char            *mergeable = candidate->mergeable;
                /* The next few line is same as
                 * if ( 0 == memcmp( mergeable, text, len ) ) return candidate;
                 *
                 * But memcmp reads more data than needed which fails asan.
                 */
                for ( size_t x = 0; x < len; x++ ) {
                        if ( text[x] != mergeable[x] ) goto next_iter;
                }
                return candidate;
        next_iter:
                (void)0;
        }
        return NULL;
}

// === BPE Algorithms ----------------------------------------------------------

/* Split the text into words.
 *
 * The entire world is "copying" OpenAI's tiktoken. It uses a quite complex
 * regexp to split text into words before applying bpe.
 *
 * There are two approaches
 * - Using a established library, e.g., pcre2, which support lookahead and
 * unicode.
 * - Writing a state machine manually. The downside is unicode support becomes
 *   a feature request later to understand \p{L} and \p{N}; in addition to
 *   correctly parse unicode byte stream.
 *
 * Here, pcre2 is used first.
 */
static error_t
tok_split_text_to_words( struct tokenizer *p, const char *text,
                         _OUT_ vec_t( size_t ) * pwords_idx )
{
        PCRE2_SPTR subject        = (PCRE2_SPTR)text;
        PCRE2_SIZE subject_length = strlen( text );

        if ( subject_length == 0 ) return OK;

        size_t i = 0;
        vec_push( pwords_idx, i );

        while ( i < subject_length ) {
                int rc = pcre2_match(
                    p->re,          /* the compiled pattern */
                    subject,        /* the subject string */
                    subject_length, /* the length of the subject */
                    i,              /* start at offset in the subject */
                    0,              /* default options */
                    p->match_data,  /* block for storing the result */
                    NULL );         /* use default match context */

                if ( rc != 1 ) {
                        EMIT_ERROR_NOTE(
                            p->ctx, "unexpected internal error: rc = %d", rc );
                        return ERROR;
                }

                PCRE2_SIZE *ovector =
                    pcre2_get_ovector_pointer( p->match_data );

                assert( ovector[0] == i );
                if ( DEBUG_PRINT )
                        LOG_DEBUG(
                            p->ctx,
                            TOK_LOGGING_PREFIX
                            "Found split word (from %2d to %2d) `%.*s`\n",
                            (int)i, (int)ovector[1], (int)( ovector[1] - i ),
                            text + i );
                i = ovector[1];
                vec_push( pwords_idx, i );
        }
        return OK;
}

error_t
tok_bpe( struct tokenizer *p, const char *text, size_t start, size_t end,
         vec_t( i64 ) * ptokens )
{
        const size_t total_len = end - start;
        /* Lookup the mergeable rank for the entire text first. */
        struct mergeable_rank *mergeable_rank = hash_search(
            p->hashes, TOK_MERGEABLE_HASH_TBL_SIZE, text + start, total_len );

        // Fast path to encode the entire text piece as one mergeable rank.
        if ( mergeable_rank != NULL ) {
                if ( DEBUG_PRINT )
                        LOG_DEBUG( p->ctx,
                                   TOK_LOGGING_PREFIX
                                   "Found mergeable_rank for whole text "
                                   "`%.*s`: `%s` %d\n",
                                   (int)( end - start ), text + start,
                                   mergeable_rank->mergeable,
                                   mergeable_rank->rank );
                vec_push( ptokens, (i64)mergeable_rank->rank );
                return OK;
        }

        /* Now try to do real bpe.
         *
         * The algorithm
         * - tries to avoid memory allocations as much as possible, and
         * - tries to avoid chopping array from the middle, which is used by
         *   many algorithms.
         *
         * To see reference code, check
         *
         *  https://github.com/eliben/code-for-blog/blob/main/2024/bpe/encode.go
         *
         * The idea, for a text
         *
         *     "abcd"
         *
         * we record the segment length as
         *
         *      1111
         *
         * It means each byte itself is a token. Next, we try to find mergeable
         * rank. We go through "ab" "bc" and "cd" and say "bc" is mergeable
         * with lowest rank. Then we update the segment length of "b" to be 2
         * (it has length 2 to cover the next byte "c").
         *
         *      1211   # the 1 for "c" is no longer used
         *
         * Then we go through the second iteration "abc", "bcd", and if "bcd"
         * can be merged with lowest rank, the segment length will be updated
         * as
         *
         *      1311   # the 1 for "d" is no longer used
         *
         * Until we cannot merge anymore.
         *
         * Then we know the final segment information is
         *
         *      13**   # the * means we can ignore them
         *
         * which means "a" itself is a token, "b" with next two bytes forms
         * next token. We emit all tokens accordingly.
         *
         * In sum, only segment length needs small memory allocation. The rest
         * is updating the data structure, no movement or copying. This is
         * possible because hash_fn and hash_search do not assume
         * NULL-terminator so we can reuse the input text buffer as much as
         * possible.
         */

        vec_t( size_t ) seg_len_vec = vec_new( );
        vec_reserve( &seg_len_vec, total_len );
        vec_set_size( seg_len_vec, total_len );

        for ( size_t i = 0; i < total_len; i++ ) seg_len_vec[i] = 1;

        /* Loop until no opportunities to merge. */
        const char *piece = text + start;
        while ( 1 ) {
                ssize_t candidate_start      = -1;
                int     candidate_rank       = TOK_MERGEALBE_RANK_COUNT + 1;
                size_t  candidate_merged_len = 0;

                size_t next_seg = 0;
                while ( next_seg < total_len ) {
                        size_t seg_start = next_seg;
                        next_seg         = seg_start + seg_len_vec[seg_start];
                        if ( next_seg >= total_len )
                                break; /* no more seg to try */

                        size_t next_seg_end = next_seg + seg_len_vec[next_seg];
                        struct mergeable_rank *mergeable_rank = hash_search(
                            p->hashes, TOK_MERGEABLE_HASH_TBL_SIZE,
                            piece + seg_start, next_seg_end - seg_start );
                        if ( mergeable_rank == NULL ) continue;

                        if ( mergeable_rank->rank < candidate_rank ) {
                                candidate_start      = (ssize_t)seg_start;
                                candidate_rank       = mergeable_rank->rank;
                                candidate_merged_len = next_seg_end - seg_start;
                        }
                }

                if ( candidate_start < 0 ) break;
                seg_len_vec[candidate_start] = candidate_merged_len;
        }

        /* Loop again to emit tokens. */
        size_t next_seg = 0;
        while ( next_seg < total_len ) {
                size_t seg_start = next_seg;
                size_t seg_end   = seg_start + seg_len_vec[seg_start];
                struct mergeable_rank *mergeable_rank =
                    hash_search( p->hashes, TOK_MERGEABLE_HASH_TBL_SIZE,
                                 piece + seg_start, seg_end - seg_start );
                assert( mergeable_rank != NULL );
                vec_push( ptokens, (i64)mergeable_rank->rank );
                if ( DEBUG_PRINT )
                        LOG_DEBUG( p->ctx,
                                   TOK_LOGGING_PREFIX
                                   "Emit partial token for seg `%.*s`: %6d",
                                   (int)( seg_end - seg_start ),
                                   piece + seg_start, mergeable_rank->rank );

                next_seg = seg_end;
        }

        vec_free( seg_len_vec );
        return OK;
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

#ifndef NDEBUG
/* A static array for the mergeable_ranks from 127990 to 127999. This table is
 * printed in the Python world so we could sanity check whether the logic here
 * is correct or not. */
static const int   tok_mergeable_ranks_starting_id = 127990;
static const char *tok_mergeable_ranks[]           = {
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
#endif

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
        char *mergeable = base64_decode( buf, (size_t)space_id );

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
        assert( rank_id >= 0 && rank_id <= TOK_MERGEALBE_RANK_COUNT );
        assert( p->mergeable_ranks[rank_id].mergeable == NULL );
        /* The model file is sequentially recorded. */
        assert( rank_id == 0 ||
                p->mergeable_ranks[rank_id - 1].mergeable != NULL );
        p->mergeable_ranks[rank_id].mergeable = _MOVED_IN_ mergeable;
        p->mergeable_ranks[rank_id].rank      = rank_id;

#ifndef NDEBUG
        /* Perform sanity check. See tok_mergeable_ranks for details. */
        if ( rank_id >= tok_mergeable_ranks_starting_id ) {
                sds_t s = sds_empty_with_cap( 200 );
                int   i;
                for ( i = 0; mergeable[i] != '\0'; i++ ) {
                        sds_cat_printf( &s, "%02x",
                                        (unsigned char)mergeable[i] );
                }
                const char *expected_piece =
                    tok_mergeable_ranks[rank_id -
                                        tok_mergeable_ranks_starting_id];
                // DEBUG( "expect %s got %s\n", expected_piece, s );
                assert( 0 == strcmp( expected_piece, s ) );
                sds_free( s );
        }
#endif
}

static error_t
tok_load_model_file( struct tokenizer *p, const char *fname )
{
        struct io_reader *r   = NULL;
        error_t           err = io_reader_open( p->ctx, fname, &r );
        if ( err != OK ) {
                EMIT_ERROR_NOTE( p->ctx,
                                 "failed to open tokenizer model file." );
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
        assert( p->mergeable_ranks[TOK_MERGEALBE_RANK_COUNT - 1].mergeable !=
                NULL );
        if ( DEBUG_PRINT )
                LOG_DEBUG( p->ctx, TOK_LOGGING_PREFIX
                           "Passed. tok_mergeable_ranks sanity check." );

cleanup:
        io_reader_close( r );
        return err;
}

error_t
tok_fill_special_tokens( struct tokenizer *tok )
{
        int                    i = 0;
        struct mergeable_rank *p;

#define ADD_SPECIAL_TOKEN( str )                \
        p            = &tok->special_tokens[i]; \
        p->mergeable = sds_new( ( str ) );      \
        p->rank      = TOK_MERGEALBE_RANK_COUNT + ( i++ )

        ADD_SPECIAL_TOKEN( "<|begin_of_text|>" );
        tok->id_begin_of_text = p->rank;

        ADD_SPECIAL_TOKEN( "<|end_of_text|>" );
        ADD_SPECIAL_TOKEN( "<|reserved_special_token_0|>" );
        ADD_SPECIAL_TOKEN( "<|reserved_special_token_1|>" );
        ADD_SPECIAL_TOKEN( "<|finetune_right_pad_id|>" );
        ADD_SPECIAL_TOKEN( "<|step_id|>" );
        ADD_SPECIAL_TOKEN( "<|start_header_id|>" );
        tok->id_start_of_header = p->rank;

        ADD_SPECIAL_TOKEN( "<|end_header_id|>" );
        tok->id_end_of_header = p->rank;

        ADD_SPECIAL_TOKEN( "<|eom_id|>" );

        ADD_SPECIAL_TOKEN( "<|eot_id|>" );
        tok->id_end_of_turn = p->rank;
        ADD_SPECIAL_TOKEN( "<|python_tag|>" );
        ADD_SPECIAL_TOKEN( "<|image|>" );

        assert( p->rank == 128011 );

#undef ADD_SPECIAL_TOKEN

        int remaining_ids = TOK_RESERVED_SPECIAL_TOKENS_COUNT - i;
        for ( int x = 0; x < remaining_ids; x++ ) {
                p       = &tok->special_tokens[i];
                sds_t s = sds_new( "<|reserved_special_token_" );
                sds_cat_printf( &s, "%d|>", 2 + x );
                p->mergeable = _MOVED_IN_ s;
                p->rank      = TOK_MERGEALBE_RANK_COUNT + ( i++ );
        }

        assert( 0 == strcmp( p->mergeable, "<|reserved_special_token_245|>" ) );
        assert( p->rank == 128255 );
        return OK;
}

static error_t
tok_compile_word_splitting_re( struct tokenizer *p )
{
        PCRE2_SPTR pattern = (const unsigned char *)TOK_SPLIT_PAT;  // TODO

        PCRE2_SIZE  erroroffset;
        int         errornumber;
        pcre2_code *re = pcre2_compile(
            pattern,               /* the pattern */
            PCRE2_ZERO_TERMINATED, /* indicates pattern is zero-terminated */
            PCRE2_UTF,             /* default options */
            &errornumber,          /* for error number */
            &erroroffset,          /* for error offset */
            NULL );                /* use default compile context */

        if ( re == NULL ) {
                PCRE2_UCHAR buffer[256];
                pcre2_get_error_message( errornumber, buffer,
                                         sizeof( buffer ) );
                EMIT_ERROR_NOTE( p->ctx,
                                 "PCRE2 compilation failed at offset %d: %s\n",
                                 (int)erroroffset, buffer );
                return ERROR;
        }

        p->re         = re;
        p->match_data = pcre2_match_data_create_from_pattern( re, NULL );
        return OK;
}

// === Implementation ------------------------------------------------------ ===
//
error_t
tok_new( struct ctx *ctx, const char *tok_model_name, struct tokenizer **pp )
{
        double            start_ms, end_ms;
        struct tokenizer *p = calloc( 1, sizeof( *p ) );
        assert( p != NULL );

        p->ctx = ctx;

        /* === --- Build mergeable_ranks. ------------------------------- === */
        start_ms    = time_ms_since_epoch( );
        error_t err = tok_load_model_file( p, tok_model_name );
        end_ms      = time_ms_since_epoch( );
        if ( err != OK ) return err;

        LOG_DEBUG(
            ctx, TOK_LOGGING_PREFIX "Took %f ms to load tokenizer model file.",
            end_ms - start_ms );

        start_ms = time_ms_since_epoch( );
        hash_init( ctx, p->mergeable_ranks, TOK_MERGEALBE_RANK_COUNT, p->hashes,
                   TOK_MERGEABLE_HASH_TBL_SIZE );
        end_ms = time_ms_since_epoch( );

        LOG_DEBUG( ctx,
                   TOK_LOGGING_PREFIX
                   "Took %f ms to build mergeable_ranks hash table.",
                   end_ms - start_ms );

        /* === --- Build special tokens.  ------------------------------- === */
        start_ms = time_ms_since_epoch( );
        err      = tok_fill_special_tokens( p );
        end_ms   = time_ms_since_epoch( );
        if ( err != OK ) return err;

        LOG_DEBUG( ctx, TOK_LOGGING_PREFIX "Took %f ms to fill special tokens.",
                   end_ms - start_ms );

        start_ms = time_ms_since_epoch( );
        hash_init( ctx, p->special_tokens, TOK_RESERVED_SPECIAL_TOKENS_COUNT,
                   p->special_tokens_hashes,
                   TOK_RESERVED_SPECIAL_TOKENS_HASH_TBL_SIZE );
        end_ms = time_ms_since_epoch( );

        LOG_DEBUG( ctx,
                   TOK_LOGGING_PREFIX
                   "Took %f ms to build special tokens hash table.",
                   end_ms - start_ms );

        /* === --- Compile word splitting regexp. ----------------------- === */
        start_ms = time_ms_since_epoch( );
        err      = tok_compile_word_splitting_re( p );
        if ( err != OK ) return err;
        end_ms = time_ms_since_epoch( );

        LOG_DEBUG( ctx, TOK_LOGGING_PREFIX "Took %f ms to compile regexp.",
                   end_ms - start_ms );

        *pp = p;
        return OK;
}

void
tok_free( struct tokenizer *p )
{
        if ( p == NULL ) return;

        for ( int i = 0; i < TOK_MERGEALBE_RANK_COUNT; i++ ) {
                free( p->mergeable_ranks[i].mergeable );
        }
        hash_deinit( p->hashes, TOK_MERGEABLE_HASH_TBL_SIZE );

        for ( int i = 0; i < TOK_RESERVED_SPECIAL_TOKENS_COUNT; i++ ) {
                sds_free( p->special_tokens[i].mergeable );
        }
        hash_deinit( p->special_tokens_hashes,
                     TOK_RESERVED_SPECIAL_TOKENS_HASH_TBL_SIZE );

        pcre2_match_data_free( p->match_data );
        pcre2_code_free( p->re );
        free( p );
}

error_t
tok_encode( struct tokenizer *p, const char *text, vec_t( i64 ) * ptokens )
{
        error_t err = OK;

        vec_t( size_t ) words_idx = vec_new( );
        err = tok_split_text_to_words( p, text, &words_idx );
        if ( err != OK ) {
                EMIT_ERROR_NOTE( p->ctx,
                                 "unexpected error during splitting words" );
                goto cleanup;
        }

        size_t id_count = vec_size( words_idx );
        if ( id_count == 0 ) goto cleanup;

        assert( id_count > 1 );  // 0 and end are inserted anyway.

        for ( size_t i = 0; i < id_count - 1; i++ ) {
                size_t start = words_idx[i];
                size_t end   = words_idx[i + 1];

                err = tok_bpe( p, text, start, end, ptokens );
                if ( err != OK ) {
                        EMIT_ERROR_NOTE(
                            p->ctx, "unexpected error during encoding words" );
                        goto cleanup;
                }
        }

cleanup:
        vec_free( words_idx );
        return err;
}

error_t
tok_encode_chat( struct tokenizer *p, const char *text, vec_t( i64 ) * ptokens )
{
        vec_push( ptokens, (i64)p->id_begin_of_text );

        /* Header for user. */
        vec_push( ptokens, (i64)p->id_start_of_header );
        tok_encode( p, "user", ptokens );
        vec_push( ptokens, (i64)p->id_end_of_header );
        tok_encode( p, "\n\n", ptokens );

        /* The real text. */
        tok_encode( p, text, ptokens );

        /* End this turn. */
        vec_push( ptokens, (i64)p->id_end_of_turn );

        /* Header for assistant. */
        vec_push( ptokens, (i64)p->id_start_of_header );
        tok_encode( p, "assistant", ptokens );
        vec_push( ptokens, (i64)p->id_end_of_header );
        tok_encode( p, "\n\n", ptokens );
        return OK;
}
