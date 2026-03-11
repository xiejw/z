pub trait Classifier {
    fn fit(&mut self, images: &[[f32; 784]], labels: &[u8]);
    fn predict(&self, image: &[f32; 784]) -> u8;
}

// ── BLAS-style SGEMM ─────────────────────────────────────────────────────────
//
// Row-major generalised matrix multiply:
//
//   C  =  alpha · op(A) · op(B)  +  beta · C
//
// where op(X) = X when trans = false, X^T when trans = true.
//
// Dimensions (after applying op):  op(A) is M×K,  op(B) is K×N,  C is M×N.
// lda / ldb / ldc = number of columns in the matrix *as stored in memory*
// (i.e. the leading dimension for row-major storage).
//
// This signature mirrors cblas_sgemm with Layout=RowMajor.
fn gemm(
    trans_a: bool,
    trans_b: bool,
    m: usize,
    n: usize,
    k: usize,
    alpha: f32,
    a: &[f32],
    lda: usize,
    b: &[f32],
    ldb: usize,
    beta: f32,
    c: &mut [f32],
    ldc: usize,
) {
    // Scale C first (handles beta = 0 cleanly to avoid NaN × 0).
    if beta == 0.0 {
        c.iter_mut().for_each(|v| *v = 0.0);
    } else if beta != 1.0 {
        c.iter_mut().for_each(|v| *v *= beta);
    }

    // Accumulate alpha · op(A) · op(B) into C.
    // Loop order (i, l, j) maximises sequential access to B and C rows when
    // trans_b = false (the common case in both forward and backward passes).
    for i in 0..m {
        for l in 0..k {
            let a_il = if !trans_a {
                a[i * lda + l]
            } else {
                a[l * lda + i]
            };
            if a_il == 0.0 {
                continue;
            }
            let alpha_a = alpha * a_il;
            let c_row = &mut c[i * ldc..i * ldc + n];
            if !trans_b {
                let b_row = &b[l * ldb..l * ldb + n];
                for j in 0..n {
                    c_row[j] += alpha_a * b_row[j];
                }
            } else {
                for j in 0..n {
                    c_row[j] += alpha_a * b[j * ldb + l];
                }
            }
        }
    }
}

// ── KNN ──────────────────────────────────────────────────────────────────────

pub struct KnnClassifier {
    k: usize,
    train: Vec<([f32; 784], u8)>,
}

impl KnnClassifier {
    pub fn new(k: usize) -> Self {
        KnnClassifier {
            k,
            train: Vec::new(),
        }
    }

    fn distance(a: &[f32; 784], b: &[f32; 784]) -> f32 {
        a.iter().zip(b.iter()).map(|(x, y)| (x - y).abs()).sum()
    }
}

impl Classifier for KnnClassifier {
    fn fit(&mut self, images: &[[f32; 784]], labels: &[u8]) {
        self.train = images
            .iter()
            .zip(labels.iter())
            .map(|(img, &lbl)| (*img, lbl))
            .collect();
    }

    fn predict(&self, image: &[f32; 784]) -> u8 {
        // Bounded max-heap of size k tracking the k nearest neighbours.
        // We store (d.to_bits(), label) as (u32, u8).  For non-negative f32,
        // IEEE 754 guarantees that bit-pattern order equals numeric order, so
        // the max-heap root is always the farthest of the k kept so far.
        use std::collections::BinaryHeap;
        let mut heap: BinaryHeap<(u32, u8)> = BinaryHeap::with_capacity(self.k + 1);
        for (train_img, train_lbl) in &self.train {
            let d = Self::distance(image, train_img);
            let key = d.to_bits();
            if heap.len() < self.k {
                heap.push((key, *train_lbl));
            } else if let Some(&(top_key, _)) = heap.peek() {
                if key < top_key {
                    heap.pop();
                    heap.push((key, *train_lbl));
                }
            }
        }
        let mut counts = [0u32; 10];
        for (_, lbl) in heap {
            counts[lbl as usize] += 1;
        }
        counts
            .iter()
            .enumerate()
            .max_by_key(|&(_, c)| c)
            .map(|(i, _)| i as u8)
            .unwrap_or(0)
    }
}

// ── Neural network ────────────────────────────────────────────────────────────
//
// Two-layer MLP:  784  →[W1,b1]→  H (ReLU)  →[W2,b2]→  10 (softmax)
//
// Training: mini-batch SGD with cross-entropy loss.
// Weights stored row-major:  W1 [H×784],  W2 [10×H].

struct Xorshift32(u32);
impl Xorshift32 {
    fn next(&mut self) -> u32 {
        self.0 ^= self.0 << 13;
        self.0 ^= self.0 >> 17;
        self.0 ^= self.0 << 5;
        self.0
    }
    /// Uniform sample in (0, 1).
    fn uniform(&mut self) -> f32 {
        (self.next() >> 8) as f32 / (1u32 << 24) as f32
    }
    /// Standard-normal sample via Box-Muller.
    fn normal(&mut self) -> f32 {
        let u1 = self.uniform().max(1e-7);
        let u2 = self.uniform();
        (-2.0 * u1.ln()).sqrt() * (2.0 * std::f32::consts::PI * u2).cos()
    }
}

pub struct NeuralNetClassifier {
    hidden: usize,
    lr: f32,
    epochs: usize,
    batch: usize,
    w1: Vec<f32>, // [H × 784]  row-major
    b1: Vec<f32>, // [H]
    w2: Vec<f32>, // [10 × H]   row-major
    b2: Vec<f32>, // [10]
}

impl NeuralNetClassifier {
    pub fn new(hidden: usize, lr: f32, epochs: usize, batch: usize) -> Self {
        NeuralNetClassifier {
            hidden,
            lr,
            epochs,
            batch,
            w1: vec![0.0; hidden * 784],
            b1: vec![0.0; hidden],
            w2: vec![0.0; 10 * hidden],
            b2: vec![0.0; 10],
        }
    }

    fn init_weights(&mut self) {
        let mut rng = Xorshift32(0xdeadbeef);
        // He initialisation: std = sqrt(2 / fan_in).
        let std1 = (2.0_f32 / 784.0).sqrt();
        let std2 = (2.0_f32 / self.hidden as f32).sqrt();
        for w in self.w1.iter_mut() {
            *w = rng.normal() * std1;
        }
        for w in self.w2.iter_mut() {
            *w = rng.normal() * std2;
        }
        // biases stay at zero
    }
}

impl Classifier for NeuralNetClassifier {
    fn fit(&mut self, images: &[[f32; 784]], labels: &[u8]) {
        self.init_weights();
        let n = images.len();
        let h = self.hidden;
        let mut rng = Xorshift32(0xcafebabe);
        let mut idx: Vec<usize> = (0..n).collect();

        for epoch in 0..self.epochs {
            // Fisher-Yates shuffle for each epoch.
            for i in (1..n).rev() {
                let j = rng.next() as usize % (i + 1);
                idx.swap(i, j);
            }

            let mut total_loss = 0.0f32;

            for chunk in idx.chunks(self.batch) {
                let b = chunk.len();

                // ── Build batch matrices ──────────────────────────────────────
                // x_batch [B × 784]  pixels already normalised to [0, 1]
                // y_batch [B × 10]   one-hot labels
                let mut x_batch = vec![0.0f32; b * 784];
                let mut y_batch = vec![0.0f32; b * 10];
                for (bi, &si) in chunk.iter().enumerate() {
                    x_batch[bi * 784..(bi + 1) * 784].copy_from_slice(&images[si]);
                    y_batch[bi * 10 + labels[si] as usize] = 1.0;
                }

                // ── Forward pass ──────────────────────────────────────────────
                // Z1 [B×H] = X [B×784] · W1^T [784×H]  +  b1
                let mut z1 = vec![0.0f32; b * h];
                gemm(
                    false, true, b, h, 784, 1.0, &x_batch, 784, &self.w1, 784, 0.0, &mut z1, h,
                );
                for i in 0..b {
                    for j in 0..h {
                        z1[i * h + j] += self.b1[j];
                    }
                }

                // A1 [B×H] = ReLU(Z1)
                let mut a1 = z1.clone();
                a1.iter_mut().for_each(|v| *v = v.max(0.0));

                // Z2 [B×10] = A1 [B×H] · W2^T [H×10]  +  b2
                let mut z2 = vec![0.0f32; b * 10];
                gemm(
                    false, true, b, 10, h, 1.0, &a1, h, &self.w2, h, 0.0, &mut z2, 10,
                );
                for i in 0..b {
                    for j in 0..10 {
                        z2[i * 10 + j] += self.b2[j];
                    }
                }

                // A2 [B×10] = softmax(Z2) row-wise
                let mut a2 = z2.clone();
                for i in 0..b {
                    let row = &mut a2[i * 10..(i + 1) * 10];
                    let max = row.iter().cloned().fold(f32::NEG_INFINITY, f32::max);
                    let mut s = 0.0f32;
                    for v in row.iter_mut() {
                        *v = (*v - max).exp();
                        s += *v;
                    }
                    for v in row.iter_mut() {
                        *v /= s;
                    }
                }

                // Accumulate cross-entropy loss for reporting.
                for i in 0..b {
                    for j in 0..10 {
                        if y_batch[i * 10 + j] > 0.5 {
                            total_loss -= a2[i * 10 + j].max(1e-7).ln();
                        }
                    }
                }

                // ── Backward pass ─────────────────────────────────────────────
                //
                // Fused softmax + cross-entropy gradient, averaged over batch:
                //   dZ2 [B×10] = (A2 − Y) / B
                let scale = 1.0 / b as f32;
                let mut dz2 = a2;
                for (dz, &y) in dz2.iter_mut().zip(y_batch.iter()) {
                    *dz = (*dz - y) * scale;
                }

                // dW2 [10×H] = dZ2^T [10×B] · A1 [B×H]
                let mut dw2 = vec![0.0f32; 10 * h];
                gemm(
                    true, false, 10, h, b, 1.0, &dz2, 10, &a1, h, 0.0, &mut dw2, h,
                );

                // db2 [10] = Σ_rows dZ2
                let mut db2 = vec![0.0f32; 10];
                for i in 0..b {
                    for j in 0..10 {
                        db2[j] += dz2[i * 10 + j];
                    }
                }

                // dA1 [B×H] = dZ2 [B×10] · W2 [10×H]
                let mut da1 = vec![0.0f32; b * h];
                gemm(
                    false, false, b, h, 10, 1.0, &dz2, 10, &self.w2, h, 0.0, &mut da1, h,
                );

                // dZ1 [B×H] = dA1 ⊙ ReLU'(Z1)
                for (dz, &z) in da1.iter_mut().zip(z1.iter()) {
                    if z <= 0.0 {
                        *dz = 0.0;
                    }
                }
                let dz1 = da1;

                // dW1 [H×784] = dZ1^T [H×B] · X [B×784]
                let mut dw1 = vec![0.0f32; h * 784];
                gemm(
                    true, false, h, 784, b, 1.0, &dz1, h, &x_batch, 784, 0.0, &mut dw1, 784,
                );

                // db1 [H] = Σ_rows dZ1
                let mut db1 = vec![0.0f32; h];
                for i in 0..b {
                    for j in 0..h {
                        db1[j] += dz1[i * h + j];
                    }
                }

                // ── SGD update ────────────────────────────────────────────────
                let lr = self.lr;
                self.w1.iter_mut().zip(dw1).for_each(|(w, d)| *w -= lr * d);
                self.b1.iter_mut().zip(db1).for_each(|(b, d)| *b -= lr * d);
                self.w2.iter_mut().zip(dw2).for_each(|(w, d)| *w -= lr * d);
                self.b2.iter_mut().zip(db2).for_each(|(b, d)| *b -= lr * d);
            }

            println!(
                "  epoch {}/{}: loss = {:.4}",
                epoch + 1,
                self.epochs,
                total_loss / n as f32
            );
        }
    }

    fn predict(&self, image: &[f32; 784]) -> u8 {
        let h = self.hidden;

        // z1 [H] = W1 [H×784] · x [784] + b1
        let mut z1 = self.b1.clone();
        for i in 0..h {
            let mut s = 0.0f32;
            for j in 0..784 {
                s += self.w1[i * 784 + j] * image[j];
            }
            z1[i] += s;
        }

        // a1 [H] = ReLU(z1)
        let a1: Vec<f32> = z1.iter().map(|&v| v.max(0.0)).collect();

        // z2 [10] = W2 [10×H] · a1 [H] + b2
        let mut z2 = self.b2.clone();
        for i in 0..10 {
            let mut s = 0.0f32;
            for j in 0..h {
                s += self.w2[i * h + j] * a1[j];
            }
            z2[i] += s;
        }

        // argmax — no softmax needed for argmax prediction
        z2.iter()
            .enumerate()
            .max_by(|a, b| a.1.partial_cmp(b.1).unwrap())
            .map(|(i, _)| i as u8)
            .unwrap_or(0)
    }
}
