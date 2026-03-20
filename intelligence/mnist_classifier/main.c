/* main.c — C CLI for MNIST classifiers
 *
 * Usage:
 *   mnist view [<index>]                          ASCII render (default 0)
 *   mnist knn  [<k>]                              KNN benchmark (default k=5)
 *   mnist nn   [hidden] [lr] [epochs] [batch]     MLP benchmark
 *
 * Data files (relative to working directory):
 *   .build/train-images-idx3-ubyte
 *   .build/train-labels-idx1-ubyte
 *   .build/t10k-images-idx3-ubyte
 *   .build/t10k-labels-idx1-ubyte
 *
 * Run `make download` once to fetch and decompress these files.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "error.h"
#include "hermes.h"
#include "par.h"

// === --- constants / paths --------------------------------------------------
// ===
//

/* 16-entry ASCII palette ordered by increasing visual density (ink coverage).
 * A normalised pixel p in [0,1] maps to index via: (int)(p * 15.0f + 0.5f) */
static const char PALETTE[16] = {
    ' ', '.', ',', ':', ';', 'i', '1', 't', 'f', 'L', 'C', 'G', '0', '8', '@', '#',
};

static const char *DATA_TRAIN_IMAGES = ".build/train-images-idx3-ubyte";
static const char *DATA_TRAIN_LABELS = ".build/train-labels-idx1-ubyte";
static const char *DATA_TEST_IMAGES  = ".build/t10k-images-idx3-ubyte";
static const char *DATA_TEST_LABELS  = ".build/t10k-labels-idx1-ubyte";

// === --- timing helper ------------------------------------------------------
// ===
//

static double elapsed_s( struct timespec t0, struct timespec t1 )
{
        return (double)( t1.tv_sec - t0.tv_sec ) +
               (double)( t1.tv_nsec - t0.tv_nsec ) * 1e-9;
}

// === --- IDX file I/O -------------------------------------------------------
// ===
//

/* Read a 4-byte big-endian uint32 from f. */
static int read_u32_be( FILE *f, uint32_t *val_out, struct err_stack *stk )
{
        unsigned char buf[4];
        if ( fread( buf, 1, 4, f ) != 4 ) {
                forge_err_emit( stk, "read_u32_be: unexpected end of file" );
                return 1;
        }
        *val_out = ( (uint32_t)buf[0] << 24 ) | ( (uint32_t)buf[1] << 16 ) |
                   ( (uint32_t)buf[2] << 8 )  |   (uint32_t)buf[3];
        return 0;
}

/* Load all images from an IDX3 file.
 * Returns a malloc'd float[n][HERMES_PIXELS] array, or NULL on error. */
static float ( *load_all_images( const char *path, size_t *n_out,
                                 struct err_stack *stk ) )[HERMES_PIXELS]
{
        int                           ok     = 0;
        FILE                         *f      = NULL;
        uint8_t                      *raw    = NULL;
        float ( *images )[HERMES_PIXELS]     = NULL;
        uint32_t                      magic  = 0, n = 0, rows = 0, cols = 0;
        size_t                        total;

        f = fopen( path, "rb" );
        if ( !f ) {
                forge_err_emit( stk, "fopen: cannot open '%s'", path );
                goto exit;
        }
        if ( read_u32_be( f, &magic, stk ) || read_u32_be( f, &n, stk ) ||
             read_u32_be( f, &rows,  stk ) || read_u32_be( f, &cols, stk ) ) {
                forge_err_emit( stk, "load_all_images: failed reading header of '%s'", path );
                goto exit;
        }
        if ( magic != 0x00000803u ) {
                forge_err_emit( stk, "load_all_images: invalid magic 0x%08x in '%s'", magic, path );
                goto exit;
        }
        if ( rows != HERMES_IMG_ROWS || cols != HERMES_IMG_COLS ) {
                forge_err_emit( stk, "load_all_images: unexpected dims %ux%u in '%s'", rows, cols, path );
                goto exit;
        }

        total  = (size_t)n * HERMES_PIXELS;
        raw    = malloc( total );
        images = malloc( (size_t)n * sizeof( *images ) );
        if ( !raw || !images ) {
                forge_err_emit( stk, "load_all_images: malloc failed" );
                goto exit;
        }
        if ( fread( raw, 1, total, f ) != total ) {
                forge_err_emit( stk, "load_all_images: short read in '%s'", path );
                goto exit;
        }

        for ( size_t img = 0; img < (size_t)n; img++ )
                for ( size_t p = 0; p < HERMES_PIXELS; p++ )
                        images[img][p] = raw[img * HERMES_PIXELS + p] / 255.0f;
        *n_out = (size_t)n;
        ok = 1;

exit:
        if ( f ) fclose( f );
        free( raw );
        if ( !ok ) { free( images ); images = NULL; }
        return images;
}

/* Load all labels from an IDX1 file.
 * Returns a malloc'd uint8_t[n] array, or NULL on error. */
static uint8_t *load_all_labels( const char *path, size_t *n_out,
                                  struct err_stack *stk )
{
        int       ok     = 0;
        FILE     *f      = NULL;
        uint8_t  *labels = NULL;
        uint32_t  magic  = 0, n = 0;

        f = fopen( path, "rb" );
        if ( !f ) {
                forge_err_emit( stk, "fopen: cannot open '%s'", path );
                goto exit;
        }
        if ( read_u32_be( f, &magic, stk ) || read_u32_be( f, &n, stk ) ) {
                forge_err_emit( stk, "load_all_labels: failed reading header of '%s'", path );
                goto exit;
        }
        if ( magic != 0x00000801u ) {
                forge_err_emit( stk, "load_all_labels: invalid magic 0x%08x in '%s'", magic, path );
                goto exit;
        }

        labels = malloc( (size_t)n );
        if ( !labels ) {
                forge_err_emit( stk, "load_all_labels: malloc failed" );
                goto exit;
        }
        if ( fread( labels, 1, (size_t)n, f ) != (size_t)n ) {
                forge_err_emit( stk, "load_all_labels: short read in '%s'", path );
                goto exit;
        }
        *n_out = (size_t)n;
        ok = 1;

exit:
        if ( f ) fclose( f );
        if ( !ok ) { free( labels ); labels = NULL; }
        return labels;
}

/* Load a single image from an IDX3 file.
 * Returns a malloc'd float[HERMES_PIXELS] array, or NULL on error. */
static float *load_image( const char *path, size_t index, struct err_stack *stk )
{
        int      ok    = 0;
        FILE    *f     = NULL;
        float   *img   = NULL;
        uint32_t magic = 0, n = 0, rows = 0, cols = 0;
        uint8_t  raw[HERMES_PIXELS];
        long     offset;

        f = fopen( path, "rb" );
        if ( !f ) {
                forge_err_emit( stk, "fopen: cannot open '%s'", path );
                goto exit;
        }
        if ( read_u32_be( f, &magic, stk ) || read_u32_be( f, &n, stk ) ||
             read_u32_be( f, &rows,  stk ) || read_u32_be( f, &cols, stk ) ) {
                forge_err_emit( stk, "load_image: failed reading header of '%s'", path );
                goto exit;
        }
        if ( magic != 0x00000803u ) {
                forge_err_emit( stk, "load_image: invalid magic in '%s'", path );
                goto exit;
        }
        if ( index >= (size_t)n ) {
                forge_err_emit( stk, "load_image: index %zu out of range (max %u) in '%s'",
                                index, n - 1, path );
                goto exit;
        }
        (void)rows;
        (void)cols;

        offset = 16L + (long)( index * HERMES_PIXELS );
        if ( fseek( f, offset, SEEK_SET ) != 0 ) {
                forge_err_emit( stk, "load_image: fseek failed in '%s'", path );
                goto exit;
        }
        if ( fread( raw, 1, HERMES_PIXELS, f ) != HERMES_PIXELS ) {
                forge_err_emit( stk, "load_image: short read in '%s'", path );
                goto exit;
        }

        img = malloc( HERMES_PIXELS * sizeof( float ) );
        if ( !img ) {
                forge_err_emit( stk, "load_image: malloc failed" );
                goto exit;
        }
        for ( int i = 0; i < HERMES_PIXELS; i++ )
                img[i] = raw[i] / 255.0f;
        ok = 1;

exit:
        if ( f ) fclose( f );
        if ( !ok ) { free( img ); img = NULL; }
        return img;
}

/* Load a single label from an IDX1 file.
 * Returns the digit (0-9) or -1 on error. */
static int load_label( const char *path, size_t index, struct err_stack *stk )
{
        int      rc    = -1;
        FILE    *f     = NULL;
        uint32_t magic = 0, n = 0;
        uint8_t  label;

        f = fopen( path, "rb" );
        if ( !f ) {
                forge_err_emit( stk, "fopen: cannot open '%s'", path );
                goto exit;
        }
        if ( read_u32_be( f, &magic, stk ) || read_u32_be( f, &n, stk ) ) {
                forge_err_emit( stk, "load_label: failed reading header of '%s'", path );
                goto exit;
        }
        if ( magic != 0x00000801u ) {
                forge_err_emit( stk, "load_label: invalid magic in '%s'", path );
                goto exit;
        }
        if ( index >= (size_t)n ) {
                forge_err_emit( stk, "load_label: index %zu out of range in '%s'", index, path );
                goto exit;
        }
        if ( fseek( f, 8L + (long)index, SEEK_SET ) != 0 ) {
                forge_err_emit( stk, "load_label: fseek failed in '%s'", path );
                goto exit;
        }
        if ( fread( &label, 1, 1, f ) != 1 ) {
                forge_err_emit( stk, "load_label: short read in '%s'", path );
                goto exit;
        }
        rc = (int)label;

exit:
        if ( f ) fclose( f );
        return rc;
}

// === --- renderer -----------------------------------------------------------
// ===
//

static void render( const float *pixels )
{
        for ( int row = 0; row < HERMES_IMG_ROWS; row++ ) {
                for ( int col = 0; col < HERMES_IMG_COLS; col++ ) {
                        float p   = pixels[row * HERMES_IMG_COLS + col];
                        int   idx = (int)( p * 15.0f + 0.5f );
                        if ( idx < 0 )  idx = 0;
                        if ( idx > 15 ) idx = 15;
                        char c = PALETTE[idx];
                        putchar( c );
                        putchar( c );
                }
                putchar( '\n' );
        }
}

// === --- evaluator ----------------------------------------------------------
// ===
//

struct eval_ctx {
        const struct hermes_classifier   *clf;
        const float ( *images )[HERMES_PIXELS];
};

static void predict_one( size_t i, void *slot_out, const void *ctx )
{
        const struct eval_ctx *e = ctx;
        *(uint8_t *)slot_out = e->clf->predict( e->clf, e->images[i] );
}

static int run_eval( const struct hermes_classifier *clf,
                     const float ( *test_images )[HERMES_PIXELS],
                     const uint8_t *test_labels, size_t n )
{
        int              rc    = 0;
        uint8_t         *preds = NULL;
        struct err_stack stk   = { 0 };
        struct eval_ctx  ctx;
        struct timespec  t0, t1;
        double           secs;

        forge_err_init( &stk );
        printf( "Evaluating on %zu test samples...\n", n );

        preds = malloc( n );
        if ( !preds ) {
                fprintf( stderr, "run_eval: malloc failed\n" );
                rc = 1;
                goto exit;
        }

        ctx = ( struct eval_ctx ){ clf, test_images };
        clock_gettime( CLOCK_MONOTONIC, &t0 );

        if ( forge_par_map( n, predict_one, preds, sizeof( uint8_t ), &ctx, 0, &stk ) ) {
                fprintf( stderr, "%s", forge_err_get( &stk ) );
                rc = 1;
                goto exit;
        }

        clock_gettime( CLOCK_MONOTONIC, &t1 );
        secs = elapsed_s( t0, t1 );
        printf( "  prediction time: %.3f s (%.0f ms)\n", secs, secs * 1000.0 );

        {
                size_t   correct           = 0;
                uint32_t class_correct[10] = { 0 };
                uint32_t class_total[10]   = { 0 };
                for ( size_t i = 0; i < n; i++ ) {
                        uint8_t lbl = test_labels[i];
                        class_total[lbl]++;
                        if ( preds[i] == lbl ) {
                                correct++;
                                class_correct[lbl]++;
                        }
                }
                printf( "\nAccuracy: %zu/%zu (%.2f%%)\n",
                        correct, n, (double)correct / (double)n * 100.0 );
                printf( "\nPer-class breakdown:\n" );
                for ( int d = 0; d < 10; d++ ) {
                        printf( "  digit %d: %u/%u (%.2f%%)\n", d,
                                class_correct[d], class_total[d],
                                (double)class_correct[d] / (double)class_total[d] * 100.0 );
                }
        }

exit:
        forge_err_deinit( &stk );
        free( preds );
        return rc;
}

// === --- entry point --------------------------------------------------------
// ===
//

static int run_knn( int argc, char **argv, struct err_stack *stk )
{
        size_t k = ( argc >= 3 ) ? (size_t)atoi( argv[2] ) : 5;

        int                             rc            = 0;
        int                             clf_init      = 0;
        float ( *train_images )[HERMES_PIXELS]        = NULL;
        uint8_t                        *train_labels  = NULL;
        float ( *test_images )[HERMES_PIXELS]         = NULL;
        uint8_t                        *test_labels   = NULL;
        size_t                          n_train       = 0, n_test = 0;
        struct hermes_knn_classifier    clf;
        struct timespec                 t0, t1;
        double                          load_s, fit_s;

        printf( "Loading training set...\n" );
        clock_gettime( CLOCK_MONOTONIC, &t0 );

        train_images = load_all_images( DATA_TRAIN_IMAGES, &n_train, stk );
        if ( !train_images ) { rc = 1; goto exit; }
        train_labels = load_all_labels( DATA_TRAIN_LABELS, &n_train, stk );
        if ( !train_labels ) { rc = 1; goto exit; }
        test_images  = load_all_images( DATA_TEST_IMAGES,  &n_test,  stk );
        if ( !test_images  ) { rc = 1; goto exit; }
        test_labels  = load_all_labels( DATA_TEST_LABELS,  &n_test,  stk );
        if ( !test_labels  ) { rc = 1; goto exit; }

        clock_gettime( CLOCK_MONOTONIC, &t1 );
        load_s = elapsed_s( t0, t1 );
        printf( "  load time: %.3f s (%.0f ms) (%zu train, %zu test)\n",
                load_s, load_s * 1000.0, n_train, n_test );

        printf( "Fitting KNN (k=%zu) on %zu training samples...\n", k, n_train );
        clock_gettime( CLOCK_MONOTONIC, &t0 );

        hermes_knn_init( &clf, k );
        clf_init = 1;
        if ( clf.ops.fit( &clf, (const float (*)[HERMES_PIXELS])train_images,
                          train_labels, n_train, stk ) ) {
                rc = 1;
                goto exit;
        }

        clock_gettime( CLOCK_MONOTONIC, &t1 );
        fit_s = elapsed_s( t0, t1 );
        printf( "  fit time:  %.3f s (%.0f ms)\n", fit_s, fit_s * 1000.0 );

        run_eval( &clf.ops, (const float (*)[HERMES_PIXELS])test_images,
                  test_labels, n_test );

exit:
        if ( clf_init ) hermes_knn_deinit( &clf );
        free( train_images );
        free( train_labels );
        free( test_images );
        free( test_labels );
        return rc;
}

static int run_nn( int argc, char **argv, struct err_stack *stk )
{
        size_t hidden = ( argc >= 3 ) ? (size_t)atoi( argv[2] ) : 128;
        float  lr     = ( argc >= 4 ) ? (float)atof( argv[3] ) : 0.1f;
        size_t epochs = ( argc >= 5 ) ? (size_t)atoi( argv[4] ) : 10;
        size_t batch  = ( argc >= 6 ) ? (size_t)atoi( argv[5] ) : 64;

        int                                    rc           = 0;
        int                                    clf_init     = 0;
        float ( *train_images )[HERMES_PIXELS]              = NULL;
        uint8_t                               *train_labels = NULL;
        float ( *test_images )[HERMES_PIXELS]               = NULL;
        uint8_t                               *test_labels  = NULL;
        size_t                                 n_train      = 0, n_test = 0;
        struct hermes_neural_net_classifier    clf;
        struct timespec                        t0, t1;
        double                                 load_s, fit_s;

        printf( "Loading training set...\n" );
        clock_gettime( CLOCK_MONOTONIC, &t0 );

        train_images = load_all_images( DATA_TRAIN_IMAGES, &n_train, stk );
        if ( !train_images ) { rc = 1; goto exit; }
        train_labels = load_all_labels( DATA_TRAIN_LABELS, &n_train, stk );
        if ( !train_labels ) { rc = 1; goto exit; }
        test_images  = load_all_images( DATA_TEST_IMAGES,  &n_test,  stk );
        if ( !test_images  ) { rc = 1; goto exit; }
        test_labels  = load_all_labels( DATA_TEST_LABELS,  &n_test,  stk );
        if ( !test_labels  ) { rc = 1; goto exit; }

        clock_gettime( CLOCK_MONOTONIC, &t1 );
        load_s = elapsed_s( t0, t1 );
        printf( "  load time: %.3f s (%.0f ms) (%zu train, %zu test)\n",
                load_s, load_s * 1000.0, n_train, n_test );

        printf( "Training MLP (hidden=%zu, lr=%.4f, epochs=%zu, batch=%zu)"
                " on %zu samples...\n",
                hidden, (double)lr, epochs, batch, n_train );
        clock_gettime( CLOCK_MONOTONIC, &t0 );

        if ( hermes_neural_net_init( &clf, hidden, lr, epochs, batch, stk ) ) {
                rc = 1;
                goto exit;
        }
        clf_init = 1;
        if ( clf.ops.fit( &clf, (const float (*)[HERMES_PIXELS])train_images,
                          train_labels, n_train, stk ) ) {
                rc = 1;
                goto exit;
        }

        clock_gettime( CLOCK_MONOTONIC, &t1 );
        fit_s = elapsed_s( t0, t1 );
        printf( "  train time: %.3f s (%.0f ms)\n", fit_s, fit_s * 1000.0 );

        run_eval( &clf.ops, (const float (*)[HERMES_PIXELS])test_images,
                  test_labels, n_test );

exit:
        if ( clf_init ) hermes_neural_net_deinit( &clf );
        free( train_images );
        free( train_labels );
        free( test_images );
        free( test_labels );
        return rc;
}

static void print_usage( void )
{
        fprintf( stderr, "Usage:\n" );
        fprintf( stderr, "  mnist view [<index>]\n" );
        fprintf( stderr, "  mnist knn  [<k>]\n" );
        fprintf( stderr, "  mnist nn   [hidden] [lr] [epochs] [batch]\n" );
}

int main( int argc, char **argv )
{
        const char *sub = ( argc >= 2 ) ? argv[1] : "view";

        struct err_stack stk = { 0 };
        forge_err_init( &stk );
        int rc = 0;

        if ( strcmp( sub, "view" ) == 0 ) {
                size_t index = ( argc >= 3 ) ? (size_t)atoi( argv[2] ) : 0;

                int label = load_label( DATA_TRAIN_LABELS, index, &stk );
                if ( label < 0 ) {
                        fprintf( stderr, "%s", forge_err_get( &stk ) );
                        rc = 1;
                        goto exit;
                }
                float *pixels = load_image( DATA_TRAIN_IMAGES, index, &stk );
                if ( !pixels ) {
                        fprintf( stderr, "%s", forge_err_get( &stk ) );
                        rc = 1;
                        goto exit;
                }
                printf( "Label: %d  (sample index %zu)\n", label, index );
                render( pixels );
                free( pixels );

        } else if ( strcmp( sub, "knn" ) == 0 ) {
                if ( run_knn( argc, argv, &stk ) ) {
                        fprintf( stderr, "%s", forge_err_get( &stk ) );
                        rc = 1;
                }

        } else if ( strcmp( sub, "nn" ) == 0 ) {
                if ( run_nn( argc, argv, &stk ) ) {
                        fprintf( stderr, "%s", forge_err_get( &stk ) );
                        rc = 1;
                }

        } else {
                fprintf( stderr, "Unknown subcommand: %s\n", sub );
                print_usage();
                rc = 1;
        }

exit:
        forge_err_deinit( &stk );
        return rc;
}
