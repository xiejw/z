pub trait Classifier {
    fn fit(&mut self, images: &[[u8; 784]], labels: &[u8]);
    fn predict(&self, image: &[u8; 784]) -> u8;
}

// ---------------------------------------------------------------------------
// KNN
// ---------------------------------------------------------------------------

pub struct KnnClassifier {
    k: usize,
    train: Vec<([u8; 784], u8)>,
}

impl KnnClassifier {
    pub fn new(k: usize) -> Self {
        KnnClassifier { k, train: Vec::new() }
    }

    fn distance(a: &[u8; 784], b: &[u8; 784]) -> u32 {
        a.iter().zip(b.iter()).map(|(&x, &y)| (x as i32 - y as i32).unsigned_abs()).sum()
    }
}

impl Classifier for KnnClassifier {
    fn fit(&mut self, images: &[[u8; 784]], labels: &[u8]) {
        self.train = images.iter().zip(labels.iter()).map(|(img, &lbl)| (*img, lbl)).collect();
    }

    fn predict(&self, image: &[u8; 784]) -> u8 {
        // Bounded max-heap of size k: store (neg_dist, label) so the heap
        // root is always the farthest of the k nearest neighbours seen so far.
        use std::collections::BinaryHeap;
        let mut heap: BinaryHeap<(i64, u8)> = BinaryHeap::with_capacity(self.k + 1);

        for (train_img, train_lbl) in &self.train {
            let d = Self::distance(image, train_img) as i64;
            let neg_d = -d;
            if heap.len() < self.k {
                heap.push((neg_d, *train_lbl));
            } else if let Some(&(top_neg, _)) = heap.peek() {
                if neg_d > top_neg {
                    heap.pop();
                    heap.push((neg_d, *train_lbl));
                }
            }
        }

        // Majority vote among the k nearest.
        let mut counts = [0u32; 10];
        for (_, lbl) in heap {
            counts[lbl as usize] += 1;
        }
        counts.iter().enumerate().max_by_key(|&(_, c)| c).map(|(i, _)| i as u8).unwrap_or(0)
    }
}
