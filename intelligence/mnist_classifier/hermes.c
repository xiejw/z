#include "hermes.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PI_F 3.14159265358979323846f

// === --- gemm ---------------------------------------------------------------
// ===
//
// Row-major generalised matrix multiply:
//
//   C = alpha · op(A) · op(B) + beta · C
//
// op(X) = X when trans=0, X^T when trans=1.
// Dimensions (after applying op): op(A) is M×K, op(B) is K×N, C is M×N.
// lda/ldb/ldc = column count of the matrix as stored in memory.
//

static void
gemm( int trans_a, int trans_b, size_t m, size_t n, size_t k, float alpha,
      const float *a, size_t lda, const float *b, size_t ldb, float beta,
      float *c, size_t ldc )
{
        /* Scale C first (handles beta=0 cleanly to avoid NaN × 0). */
        if ( beta == 0.0f ) {
                for ( size_t i = 0; i < m * ldc; i++ ) c[i] = 0.0f;
        } else if ( beta != 1.0f ) {
                for ( size_t i = 0; i < m * ldc; i++ ) c[i] *= beta;
        }

        /* Accumulate alpha · op(A) · op(B) into C.
         * Loop order (i, l, j) maximises sequential access to B and C rows
         * when trans_b=0 (the common case in forward and backward passes). */
        for ( size_t i = 0; i < m; i++ ) {
                for ( size_t l = 0; l < k; l++ ) {
                        float a_il = trans_a ? a[l * lda + i] : a[i * lda + l];
                        if ( a_il == 0.0f ) continue;
                        float  alpha_a = alpha * a_il;
                        float *c_row   = c + i * ldc;
                        if ( !trans_b ) {
                                const float *b_row = b + l * ldb;
                                for ( size_t j = 0; j < n; j++ )
                                        c_row[j] += alpha_a * b_row[j];
                        } else {
                                for ( size_t j = 0; j < n; j++ )
                                        c_row[j] += alpha_a * b[j * ldb + l];
                        }
                }
        }
}

// === --- RNG ----------------------------------------------------------------
// ===
//
// Xorshift32 state is a plain uint32_t passed by pointer.
//

static uint32_t
xorshift32_next( uint32_t *state )
{
        *state ^= *state << 13;
        *state ^= *state >> 17;
        *state ^= *state << 5;
        return *state;
}

/* Uniform sample in (0, 1). */
static float
xorshift32_uniform( uint32_t *state )
{
        return (float)( xorshift32_next( state ) >> 8 ) / (float)( 1u << 24 );
}

/* Standard-normal sample via Box-Muller. */
static float
xorshift32_normal( uint32_t *state )
{
        float u1 = xorshift32_uniform( state );
        if ( u1 < 1e-7f ) u1 = 1e-7f;
        float u2 = xorshift32_uniform( state );
        return sqrtf( -2.0f * logf( u1 ) ) * cosf( 2.0f * PI_F * u2 );
}

// === --- KNN ----------------------------------------------------------------
// ===
//

/* Max-heap helpers operating on a flat uint64_t array.
 * Each entry packs (dist_bits_u32 << 32) | label.
 * For non-negative f32, IEEE 754 bit-pattern order equals numeric order,
 * so the max-heap root is always the farthest of the k kept so far. */

static void
heap_push( uint64_t *heap, size_t *n, uint64_t val )
{
        size_t i = ( *n )++;
        heap[i]  = val;
        while ( i > 0 ) {
                size_t parent = ( i - 1 ) >> 1;
                if ( heap[parent] >= heap[i] ) break;
                uint64_t t   = heap[parent];
                heap[parent] = heap[i];
                heap[i]      = t;
                i            = parent;
        }
}

static void
heap_pop( uint64_t *heap, size_t *n )
{
        heap[0]  = heap[--( *n )];
        size_t i = 0;
        for ( ;; ) {
                size_t l = 2 * i + 1, r = 2 * i + 2, m = i;
                if ( l < *n && heap[l] > heap[m] ) m = l;
                if ( r < *n && heap[r] > heap[m] ) m = r;
                if ( m == i ) break;
                uint64_t t = heap[m];
                heap[m]    = heap[i];
                heap[i]    = t;
                i          = m;
        }
}

static float
knn_l1_distance( const float *a, const float *b )
{
        float s = 0.0f;
        for ( size_t i = 0; i < HERMES_PIXELS; i++ ) {
                float d = a[i] - b[i];
                s += d < 0.0f ? -d : d;
        }
        return s;
}

static int
knn_fit( void          *self, const float ( *images )[HERMES_PIXELS],
         const uint8_t *labels, size_t n, struct err_stack *stk )
{
        struct hermes_knn_classifier *c = self;

        free( c->train_images );
        free( c->train_labels );
        c->train_images = NULL;
        c->train_labels = NULL;
        c->train_n      = 0;

        c->train_images = malloc( n * sizeof( *c->train_images ) );
        if ( !c->train_images ) {
                forge_err_emit( stk,
                                "knn_fit: malloc failed for train_images" );
                return 1;
        }
        c->train_labels = malloc( n * sizeof( uint8_t ) );
        if ( !c->train_labels ) {
                free( c->train_images );
                c->train_images = NULL;
                forge_err_emit( stk,
                                "knn_fit: malloc failed for train_labels" );
                return 1;
        }

        memcpy( c->train_images, images, n * sizeof( *c->train_images ) );
        memcpy( c->train_labels, labels, n * sizeof( uint8_t ) );
        c->train_n = n;
        return 0;
}

static uint8_t
knn_predict( const void *self, const float image[HERMES_PIXELS] )
{
        const struct hermes_knn_classifier *c = self;
        size_t                              k = c->k;

        uint64_t *heap      = malloc( k * sizeof( uint64_t ) );
        size_t    heap_size = 0;
        if ( !heap ) return 0;

        for ( size_t t = 0; t < c->train_n; t++ ) {
                float    d = knn_l1_distance( image, c->train_images[t] );
                uint32_t d_bits;
                memcpy( &d_bits, &d, sizeof( float ) );
                uint64_t entry =
                    ( (uint64_t)d_bits << 32 ) | c->train_labels[t];

                if ( heap_size < k ) {
                        heap_push( heap, &heap_size, entry );
                } else {
                        uint32_t top_bits;
                        uint64_t top = heap[0];
                        top_bits     = (uint32_t)( top >> 32 );
                        if ( d_bits < top_bits ) {
                                heap_pop( heap, &heap_size );
                                heap_push( heap, &heap_size, entry );
                        }
                }
        }

        uint32_t counts[HERMES_CLASSES] = { 0 };
        for ( size_t i = 0; i < heap_size; i++ )
                counts[(uint8_t)( heap[i] & 0xFF )]++;

        free( heap );

        uint8_t best = 0;
        for ( size_t i = 1; i < HERMES_CLASSES; i++ )
                if ( counts[i] > counts[best] ) best = (uint8_t)i;
        return best;
}

void
hermes_knn_init( struct hermes_knn_classifier *c, size_t k )
{
        c->ops.fit      = knn_fit;
        c->ops.predict  = knn_predict;
        c->k            = k;
        c->train_images = NULL;
        c->train_labels = NULL;
        c->train_n      = 0;
}

void
hermes_knn_deinit( struct hermes_knn_classifier *c )
{
        free( c->train_images );
        free( c->train_labels );
        c->train_images = NULL;
        c->train_labels = NULL;
        c->train_n      = 0;
}

// === --- Neural network -----------------------------------------------------
// ===
//
// Two-layer MLP: 784 →[W1,b1]→ H (ReLU) →[W2,b2]→ 10 (softmax)
// Training: mini-batch SGD with cross-entropy loss.
//

static void
nn_init_weights( struct hermes_neural_net_classifier *c )
{
        uint32_t rng  = 0xdeadbeef;
        float    std1 = sqrtf( 2.0f / (float)HERMES_PIXELS );
        float    std2 = sqrtf( 2.0f / (float)c->hidden );

        /* He initialisation: std = sqrt(2 / fan_in); biases stay zero. */
        for ( size_t i = 0; i < c->hidden * HERMES_PIXELS; i++ )
                c->w1[i] = xorshift32_normal( &rng ) * std1;
        for ( size_t i = 0; i < HERMES_CLASSES * c->hidden; i++ )
                c->w2[i] = xorshift32_normal( &rng ) * std2;
}

static int
nn_fit( void          *self, const float ( *images )[HERMES_PIXELS],
        const uint8_t *labels, size_t n, struct err_stack *stk )
{
        struct hermes_neural_net_classifier *c = self;
        size_t                               h = c->hidden;

        nn_init_weights( c );

        size_t *idx = malloc( n * sizeof( size_t ) );
        if ( !idx ) {
                forge_err_emit( stk,
                                "nn_fit: malloc failed for shuffle index" );
                return 1;
        }
        for ( size_t i = 0; i < n; i++ ) idx[i] = i;

        uint32_t rng = 0xcafebabe;

        for ( size_t epoch = 0; epoch < c->epochs; epoch++ ) {
                /* Fisher-Yates shuffle. */
                for ( size_t i = n - 1; i > 0; i-- ) {
                        size_t j = (size_t)xorshift32_next( &rng ) % ( i + 1 );
                        size_t t = idx[i];
                        idx[i]   = idx[j];
                        idx[j]   = t;
                }

                float total_loss = 0.0f;

                for ( size_t start = 0; start < n; start += c->batch ) {
                        size_t b = c->batch;
                        if ( start + b > n ) b = n - start;
                        const size_t *chunk = idx + start;

                        /* Allocate per-batch temporaries. */
                        float *x_batch =
                            malloc( b * HERMES_PIXELS * sizeof( float ) );
                        float *y_batch =
                            calloc( b * HERMES_CLASSES, sizeof( float ) );
                        float *z1 = malloc( b * h * sizeof( float ) );
                        float *a1 = malloc( b * h * sizeof( float ) );
                        float *z2 =
                            malloc( b * HERMES_CLASSES * sizeof( float ) );
                        float *a2 =
                            malloc( b * HERMES_CLASSES * sizeof( float ) );
                        float *dz2 =
                            malloc( b * HERMES_CLASSES * sizeof( float ) );
                        float *dw2 =
                            calloc( HERMES_CLASSES * h, sizeof( float ) );
                        float *db2 = calloc( HERMES_CLASSES, sizeof( float ) );
                        float *da1 = malloc( b * h * sizeof( float ) );
                        float *dw1 =
                            calloc( h * HERMES_PIXELS, sizeof( float ) );
                        float *db1 = calloc( h, sizeof( float ) );

                        if ( !x_batch || !y_batch || !z1 || !a1 || !z2 || !a2 ||
                             !dz2 || !dw2 || !db2 || !da1 || !dw1 || !db1 ) {
                                free( x_batch );
                                free( y_batch );
                                free( z1 );
                                free( a1 );
                                free( z2 );
                                free( a2 );
                                free( dz2 );
                                free( dw2 );
                                free( db2 );
                                free( da1 );
                                free( dw1 );
                                free( db1 );
                                free( idx );
                                forge_err_emit( stk,
                                                "nn_fit: malloc failed for "
                                                "batch temporaries" );
                                return 1;
                        }

                        /* ── Build batch matrices
                         * ─────────────────────────────────────── */
                        /* x_batch [B×784]: pixels already normalised to [0,1]
                         */
                        /* y_batch [B×10]:  one-hot labels */
                        for ( size_t bi = 0; bi < b; bi++ ) {
                                size_t si = chunk[bi];
                                memcpy( x_batch + bi * HERMES_PIXELS,
                                        images[si],
                                        HERMES_PIXELS * sizeof( float ) );
                                y_batch[bi * HERMES_CLASSES + labels[si]] =
                                    1.0f;
                        }

                        /* ── Forward pass
                         * ─────────────────────────────────────────────── */
                        /* Z1 [B×H] = X [B×784] · W1^T [784×H] + b1 */
                        gemm( 0, 1, b, h, HERMES_PIXELS, 1.0f, x_batch,
                              HERMES_PIXELS, c->w1, HERMES_PIXELS, 0.0f, z1,
                              h );
                        for ( size_t i = 0; i < b; i++ )
                                for ( size_t j = 0; j < h; j++ )
                                        z1[i * h + j] += c->b1[j];

                        /* A1 [B×H] = ReLU(Z1) */
                        memcpy( a1, z1, b * h * sizeof( float ) );
                        for ( size_t i = 0; i < b * h; i++ )
                                if ( a1[i] < 0.0f ) a1[i] = 0.0f;

                        /* Z2 [B×10] = A1 [B×H] · W2^T [H×10] + b2 */
                        gemm( 0, 1, b, HERMES_CLASSES, h, 1.0f, a1, h, c->w2, h,
                              0.0f, z2, HERMES_CLASSES );
                        for ( size_t i = 0; i < b; i++ )
                                for ( size_t j = 0; j < HERMES_CLASSES; j++ )
                                        z2[i * HERMES_CLASSES + j] += c->b2[j];

                        /* A2 [B×10] = softmax(Z2) row-wise */
                        memcpy( a2, z2, b * HERMES_CLASSES * sizeof( float ) );
                        for ( size_t i = 0; i < b; i++ ) {
                                float *row = a2 + i * HERMES_CLASSES;
                                float  max = row[0];
                                for ( size_t j = 1; j < HERMES_CLASSES; j++ )
                                        if ( row[j] > max ) max = row[j];
                                float s = 0.0f;
                                for ( size_t j = 0; j < HERMES_CLASSES; j++ ) {
                                        row[j] = expf( row[j] - max );
                                        s += row[j];
                                }
                                for ( size_t j = 0; j < HERMES_CLASSES; j++ )
                                        row[j] /= s;
                        }

                        /* Accumulate cross-entropy loss. */
                        for ( size_t i = 0; i < b; i++ ) {
                                for ( size_t j = 0; j < HERMES_CLASSES; j++ ) {
                                        if ( y_batch[i * HERMES_CLASSES + j] >
                                             0.5f ) {
                                                float p =
                                                    a2[i * HERMES_CLASSES + j];
                                                if ( p < 1e-7f ) p = 1e-7f;
                                                total_loss -= logf( p );
                                        }
                                }
                        }

                        /* ── Backward pass
                         * ────────────────────────────────────────────── */
                        /* Fused softmax + cross-entropy gradient, averaged over
                         * batch: dZ2 [B×10] = (A2 − Y) / B */
                        float scale = 1.0f / (float)b;
                        for ( size_t i = 0; i < b * HERMES_CLASSES; i++ )
                                dz2[i] = ( a2[i] - y_batch[i] ) * scale;

                        /* dW2 [10×H] = dZ2^T [10×B] · A1 [B×H] */
                        gemm( 1, 0, HERMES_CLASSES, h, b, 1.0f, dz2,
                              HERMES_CLASSES, a1, h, 0.0f, dw2, h );

                        /* db2 [10] = Σ_rows dZ2 */
                        for ( size_t i = 0; i < b; i++ )
                                for ( size_t j = 0; j < HERMES_CLASSES; j++ )
                                        db2[j] += dz2[i * HERMES_CLASSES + j];

                        /* dA1 [B×H] = dZ2 [B×10] · W2 [10×H] */
                        gemm( 0, 0, b, h, HERMES_CLASSES, 1.0f, dz2,
                              HERMES_CLASSES, c->w2, h, 0.0f, da1, h );

                        /* dZ1 [B×H] = dA1 ⊙ ReLU'(Z1)  (in-place; da1 becomes
                         * dz1) */
                        for ( size_t i = 0; i < b * h; i++ )
                                if ( z1[i] <= 0.0f ) da1[i] = 0.0f;
                        float *dz1 = da1;

                        /* dW1 [H×784] = dZ1^T [H×B] · X [B×784] */
                        gemm( 1, 0, h, HERMES_PIXELS, b, 1.0f, dz1, h, x_batch,
                              HERMES_PIXELS, 0.0f, dw1, HERMES_PIXELS );

                        /* db1 [H] = Σ_rows dZ1 */
                        for ( size_t i = 0; i < b; i++ )
                                for ( size_t j = 0; j < h; j++ )
                                        db1[j] += dz1[i * h + j];

                        /* ── SGD update
                         * ─────────────────────────────────────────────── */
                        float lr = c->lr;
                        for ( size_t i = 0; i < h * HERMES_PIXELS; i++ )
                                c->w1[i] -= lr * dw1[i];
                        for ( size_t i = 0; i < h; i++ )
                                c->b1[i] -= lr * db1[i];
                        for ( size_t i = 0; i < HERMES_CLASSES * h; i++ )
                                c->w2[i] -= lr * dw2[i];
                        for ( size_t i = 0; i < HERMES_CLASSES; i++ )
                                c->b2[i] -= lr * db2[i];

                        /* Free per-batch temporaries. */
                        free( x_batch );
                        free( y_batch );
                        free( z1 );
                        free( a1 );
                        free( z2 );
                        free( a2 );
                        free( dz2 );
                        free( dw2 );
                        free( db2 );
                        free( da1 );
                        free( dw1 );
                        free( db1 );
                }

                printf( "  epoch %zu/%zu: loss = %.4f\n", epoch + 1, c->epochs,
                        total_loss / (float)n );
        }

        free( idx );
        return 0;
}

static uint8_t
nn_predict( const void *self, const float image[HERMES_PIXELS] )
{
        const struct hermes_neural_net_classifier *c = self;
        size_t                                     h = c->hidden;

        /* a1 [H] = ReLU(W1 [H×784] · x [784] + b1) */
        float *a1 = malloc( h * sizeof( float ) );
        if ( !a1 ) return 0;
        for ( size_t i = 0; i < h; i++ ) {
                float s = c->b1[i];
                for ( size_t j = 0; j < HERMES_PIXELS; j++ )
                        s += c->w1[i * HERMES_PIXELS + j] * image[j];
                a1[i] = s > 0.0f ? s : 0.0f;
        }

        /* z2 [10] = W2 [10×H] · a1 [H] + b2; argmax — no softmax needed. */
        uint8_t best     = 0;
        float   best_val = c->b2[0];
        for ( size_t j = 0; j < h; j++ ) best_val += c->w2[0 * h + j] * a1[j];

        for ( size_t i = 1; i < HERMES_CLASSES; i++ ) {
                float v = c->b2[i];
                for ( size_t j = 0; j < h; j++ ) v += c->w2[i * h + j] * a1[j];
                if ( v > best_val ) {
                        best_val = v;
                        best     = (uint8_t)i;
                }
        }

        free( a1 );
        return best;
}

void
hermes_neural_net_deinit( struct hermes_neural_net_classifier *c )
{
        free( c->w1 );
        c->w1 = NULL;
        free( c->b1 );
        c->b1 = NULL;
        free( c->w2 );
        c->w2 = NULL;
        free( c->b2 );
        c->b2 = NULL;
}

int
hermes_neural_net_init( struct hermes_neural_net_classifier *c, size_t hidden,
                        float lr, size_t epochs, size_t batch,
                        struct err_stack *stk )
{
        c->ops.fit     = nn_fit;
        c->ops.predict = nn_predict;
        c->hidden      = hidden;
        c->lr          = lr;
        c->epochs      = epochs;
        c->batch       = batch;
        c->w1          = calloc( hidden * HERMES_PIXELS, sizeof( float ) );
        c->b1          = calloc( hidden, sizeof( float ) );
        c->w2          = calloc( HERMES_CLASSES * hidden, sizeof( float ) );
        c->b2          = calloc( HERMES_CLASSES, sizeof( float ) );

        if ( !c->w1 || !c->b1 || !c->w2 || !c->b2 ) {
                hermes_neural_net_deinit( c );
                forge_err_emit( stk, "hermes_neural_net_init: malloc failed" );
                return 1;
        }
        return 0;
}
