/* hermes.h — KNN and two-layer MLP classifiers
 *
 * Namespace rules:
 *   hermes_  — ML classifier types and functions
 *   forge_   — infrastructure utilities (err_stack)
 *   HERMES_  — ML constants
 */

#ifndef HERMES_H
#define HERMES_H

#include <stddef.h>
#include <stdint.h>

#include "error.h"

// === --- Constants ----------------------------------------------------------
// ===
//

#define HERMES_PIXELS   784 /* 28 × 28 pixels per image */
#define HERMES_CLASSES  10  /* digits 0–9               */
#define HERMES_IMG_ROWS 28
#define HERMES_IMG_COLS 28

// === --- Classifier vtable --------------------------------------------------
// ===
//

struct hermes_classifier {
        /* fit: return 0 on success, 1 on error (written into stk) */
        int ( *fit )( void *self, const float ( *images )[HERMES_PIXELS],
                      const uint8_t *labels, size_t n, struct err_stack *stk );
        uint8_t ( *predict )( const void *self,
                              const float image[HERMES_PIXELS] );
};

// === --- KNN classifier -----------------------------------------------------
// ===
//

struct hermes_knn_classifier {
        struct hermes_classifier ops; /* MUST be first */
        size_t                   k;
        float ( *train_images )[HERMES_PIXELS];
        uint8_t *train_labels;
        size_t   train_n;
};

void hermes_knn_init( struct hermes_knn_classifier *c, size_t k );
void hermes_knn_deinit( struct hermes_knn_classifier *c );

// === --- Neural-net classifier ----------------------------------------------
// ===
//
// Two-layer MLP: 784 →[W1,b1]→ H (ReLU) →[W2,b2]→ 10 (softmax)
// Weights row-major: W1 [H×784], W2 [10×H].
//

struct hermes_neural_net_classifier {
        struct hermes_classifier ops; /* MUST be first */
        size_t                   hidden;
        float                    lr;
        size_t                   epochs;
        size_t                   batch;
        float                   *w1; /* [hidden × HERMES_PIXELS] row-major */
        float                   *b1; /* [hidden]                           */
        float                   *w2; /* [HERMES_CLASSES × hidden] row-major */
        float                   *b2; /* [HERMES_CLASSES]                   */
};

/* Returns 0 on success, 1 on malloc failure (error written into stk). */
int  hermes_neural_net_init( struct hermes_neural_net_classifier *c,
                             size_t hidden, float lr, size_t epochs,
                             size_t batch, struct err_stack *stk );
void hermes_neural_net_deinit( struct hermes_neural_net_classifier *c );

#endif /* HERMES_H */
