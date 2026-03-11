// MNIST terminal viewer + classifiers
//
// hermes:v1
//
// Usage:
//   cargo run -- view [<index>]                          ASCII render (default index 0)
//   cargo run --release -- knn [<k>]                     KNN accuracy benchmark (default k=5)
//   cargo run --release -- nn [hidden] [lr] [ep] [batch] MLP accuracy benchmark
//
// Data files (relative to the working directory):
//   .build/train-images-idx3-ubyte   — 60 000 × 28 × 28 grayscale pixel arrays
//   .build/train-labels-idx1-ubyte   — 60 000 digit labels (0–9)
//   .build/t10k-images-idx3-ubyte    — 10 000 × 28 × 28 test pixel arrays
//   .build/t10k-labels-idx1-ubyte    — 10 000 test digit labels (0–9)
//
// Run `make download` once to fetch and decompress these files before use.
//
mod hermes;

use hermes::Classifier as _;
use rayon::prelude::*;
use std::fs::File;
use std::io::{Read, Seek, SeekFrom};
use std::time::Instant;

fn fmt_elapsed(t: Instant) -> String {
    let ms = t.elapsed().as_millis();
    format!("{:.3} s ({} ms)", ms as f64 / 1000.0, ms)
}

// 16-entry ASCII palette ordered by increasing visual density (ink coverage).
// A pixel value 0–255 maps to an index via: pixel * 15 / 255
//   index  0  ' '  — empty, background (pixel ≈ 0, black in MNIST)
//   index  1  '.'  — faint dot
//   index  2  ','  — slightly heavier dot
//   index  3  ':'  — two-dot vertical
//   index  4  ';'  — two-dot with tail
//   index  5  'i'  — thin vertical stroke
//   index  6  '1'  — slightly wider stroke
//   index  7  't'  — stroke with crossbar
//   index  8  'f'  — stroke with top hook
//   index  9  'L'  — right-angle stroke
//   index 10  'C'  — open curve, medium density
//   index 11  'G'  — closed curve, heavier
//   index 12  '0'  — oval, dense outline
//   index 13  '8'  — double oval, very dense
//   index 14  '@'  — filled oval with inner detail
//   index 15  '#'  — fully filled block (pixel ≈ 255, white in MNIST)
const PALETTE: &[char] = &[
    ' ', '.', ',', ':', ';', 'i', '1', 't', 'f', 'L', 'C', 'G', '0', '8', '@', '#',
];
const ROWS: usize = 28;
const COLS: usize = 28;

fn read_u32_be(file: &mut File) -> std::io::Result<u32> {
    let mut buf = [0u8; 4];
    file.read_exact(&mut buf)?;
    Ok(u32::from_be_bytes(buf))
}

fn load_image(path: &str, index: usize) -> std::io::Result<Vec<f32>> {
    // IDX3 image file layout (all integers big-endian):
    //   bytes  0– 3: magic = 0x00000803  (0x08 = u8 data type, 0x03 = 3 dimensions)
    //   bytes  4– 7: number of images
    //   bytes  8–11: number of rows    (28 for MNIST)
    //   bytes 12–15: number of cols    (28 for MNIST)
    //   bytes 16+  : raw pixel data, row-major, one u8 per pixel (0=black, 255=white)
    //                image N starts at offset 16 + N * rows * cols
    let mut f = File::open(path)?;
    let magic = read_u32_be(&mut f)?;
    assert_eq!(
        magic, 0x0000_0803,
        "invalid images magic number; likely the file is bad"
    );
    let num_images = read_u32_be(&mut f)? as usize;
    let rows = read_u32_be(&mut f)? as usize;
    let cols = read_u32_be(&mut f)? as usize;
    assert!(
        index < num_images,
        "index {index} out of range (max {})",
        num_images - 1
    );
    assert_eq!(rows, ROWS, "unexpected row count in file");
    assert_eq!(cols, COLS, "unexpected col count in file");
    let offset = 16 + index * ROWS * COLS;
    f.seek(SeekFrom::Start(offset as u64))?;
    let mut raw = vec![0u8; ROWS * COLS];
    f.read_exact(&mut raw)?;
    Ok(raw.iter().map(|&p| p as f32 / 255.0).collect())
}

fn load_label(path: &str, index: usize) -> std::io::Result<u8> {
    // IDX1 label file layout (all integers big-endian):
    //   bytes 0–3: magic = 0x00000801  (0x08 = u8 data type, 0x01 = 1 dimension)
    //   bytes 4–7: number of labels
    //   bytes 8+ : one u8 per label (digit class 0–9)
    //              label N is at offset 8 + N
    let mut f = File::open(path)?;
    let magic = read_u32_be(&mut f)?;
    assert_eq!(
        magic, 0x0000_0801,
        "invalid labels magic number; likely the file is bad"
    );
    let num_labels = read_u32_be(&mut f)? as usize;
    assert!(
        index < num_labels,
        "index {index} out of range (max {})",
        num_labels - 1
    );
    f.seek(SeekFrom::Start((8 + index) as u64))?;
    let mut label = [0u8; 1];
    f.read_exact(&mut label)?;
    Ok(label[0])
}

fn load_all_images(path: &str) -> std::io::Result<Vec<[f32; 784]>> {
    let mut f = File::open(path)?;
    let magic = read_u32_be(&mut f)?;
    assert_eq!(magic, 0x0000_0803, "invalid images magic number");
    let num_images = read_u32_be(&mut f)? as usize;
    let rows = read_u32_be(&mut f)? as usize;
    let cols = read_u32_be(&mut f)? as usize;
    assert_eq!(rows, ROWS);
    assert_eq!(cols, COLS);
    let mut raw = vec![0u8; num_images * ROWS * COLS];
    f.read_exact(&mut raw)?;
    Ok(raw
        .chunks_exact(784)
        .map(|c| {
            let mut a = [0.0f32; 784];
            for (dst, &src) in a.iter_mut().zip(c.iter()) {
                *dst = src as f32 / 255.0;
            }
            a
        })
        .collect())
}

fn load_all_labels(path: &str) -> std::io::Result<Vec<u8>> {
    let mut f = File::open(path)?;
    let magic = read_u32_be(&mut f)?;
    assert_eq!(magic, 0x0000_0801, "invalid labels magic number");
    let num_labels = read_u32_be(&mut f)? as usize;
    let mut labels = vec![0u8; num_labels];
    f.read_exact(&mut labels)?;
    Ok(labels)
}

fn render(pixels: &[f32]) {
    for row in 0..ROWS {
        let line: String = pixels[row * COLS..(row + 1) * COLS]
            .iter()
            .flat_map(|&p| {
                let idx = (p * (PALETTE.len() - 1) as f32).round() as usize;
                let ch = PALETTE[idx];
                [ch, ch]
            })
            .collect();
        println!("{}", line);
    }
}

/// Evaluate a trained classifier on test_images/test_labels, printing accuracy.
/// Predictions run in parallel via rayon.
fn run_eval(
    clf: &(impl hermes::Classifier + Sync),
    test_images: &[[f32; 784]],
    test_labels: &[u8],
) {
    let n = test_images.len();
    println!("Evaluating on {n} test samples...");
    let t_eval = Instant::now();
    let results: Vec<(u8, u8)> = test_images
        .par_iter()
        .zip(test_labels.par_iter())
        .map(|(img, &lbl)| (clf.predict(img), lbl))
        .collect();
    println!("  prediction time: {}", fmt_elapsed(t_eval));

    let mut correct = 0usize;
    let mut class_correct = [0u32; 10];
    let mut class_total = [0u32; 10];
    for (pred, true_lbl) in results {
        class_total[true_lbl as usize] += 1;
        if pred == true_lbl {
            correct += 1;
            class_correct[true_lbl as usize] += 1;
        }
    }
    println!(
        "\nAccuracy: {correct}/{n} ({:.2}%)",
        correct as f64 / n as f64 * 100.0
    );
    println!("\nPer-class breakdown:");
    for digit in 0..10usize {
        let tot = class_total[digit];
        let cor = class_correct[digit];
        println!(
            "  digit {digit}: {cor}/{tot} ({:.2}%)",
            cor as f64 / tot as f64 * 100.0
        );
    }
}

fn main() {
    let train_images_path = ".build/train-images-idx3-ubyte";
    let train_labels_path = ".build/train-labels-idx1-ubyte";
    let test_images_path = ".build/t10k-images-idx3-ubyte";
    let test_labels_path = ".build/t10k-labels-idx1-ubyte";

    let mut args = std::env::args().skip(1);
    let subcommand = args.next().unwrap_or_else(|| "view".to_string());

    match subcommand.as_str() {
        "view" => {
            let index: usize = args.next().and_then(|s| s.parse().ok()).unwrap_or(0);
            let label = load_label(train_labels_path, index).expect("failed to load label");
            let pixels = load_image(train_images_path, index).expect("failed to load image");
            println!("Label: {label}  (sample index {index})");
            render(&pixels);
        }

        "knn" => {
            let k: usize = args.next().and_then(|s| s.parse().ok()).unwrap_or(5);

            println!("Loading training set...");
            let t_load = Instant::now();
            let train_images =
                load_all_images(train_images_path).expect("failed to load train images");
            let train_labels =
                load_all_labels(train_labels_path).expect("failed to load train labels");
            let test_images =
                load_all_images(test_images_path).expect("failed to load test images");
            let test_labels =
                load_all_labels(test_labels_path).expect("failed to load test labels");
            println!(
                "  load time: {} ({} train, {} test)",
                fmt_elapsed(t_load),
                train_images.len(),
                test_images.len()
            );

            println!(
                "Fitting KNN (k={k}) on {} training samples...",
                train_images.len()
            );
            let t_fit = Instant::now();
            let mut clf = hermes::KnnClassifier::new(k);
            clf.fit(&train_images, &train_labels);
            println!("  fit time:  {}", fmt_elapsed(t_fit));

            run_eval(&clf, &test_images, &test_labels);
        }

        "nn" => {
            let hidden: usize = args.next().and_then(|s| s.parse().ok()).unwrap_or(128);
            let lr: f32 = args.next().and_then(|s| s.parse().ok()).unwrap_or(0.1);
            let epochs: usize = args.next().and_then(|s| s.parse().ok()).unwrap_or(10);
            let batch: usize = args.next().and_then(|s| s.parse().ok()).unwrap_or(64);

            println!("Loading training set...");
            let t_load = Instant::now();
            let train_images =
                load_all_images(train_images_path).expect("failed to load train images");
            let train_labels =
                load_all_labels(train_labels_path).expect("failed to load train labels");
            let test_images =
                load_all_images(test_images_path).expect("failed to load test images");
            let test_labels =
                load_all_labels(test_labels_path).expect("failed to load test labels");
            println!(
                "  load time: {} ({} train, {} test)",
                fmt_elapsed(t_load),
                train_images.len(),
                test_images.len()
            );

            println!(
                "Training MLP (hidden={hidden}, lr={lr}, epochs={epochs}, batch={batch}) on {} samples...",
                train_images.len()
            );
            let t_fit = Instant::now();
            let mut clf = hermes::NeuralNetClassifier::new(hidden, lr, epochs, batch);
            clf.fit(&train_images, &train_labels);
            println!("  train time: {}", fmt_elapsed(t_fit));

            run_eval(&clf, &test_images, &test_labels);
        }

        other => {
            eprintln!("Unknown subcommand: {other:?}");
            eprintln!("Usage:");
            eprintln!("  cargo run -- view [<index>]");
            eprintln!("  cargo run --release -- knn [<k>]");
            eprintln!("  cargo run --release -- nn [hidden] [lr] [epochs] [batch]");
            std::process::exit(1);
        }
    }
}
