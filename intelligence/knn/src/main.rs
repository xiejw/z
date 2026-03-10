// MNIST terminal viewer + KNN classifier
//
// hermes:v1
//
// Usage:
//   cargo run -- view [<index>]      Render sample at index as ASCII art (default 0)
//   cargo run --release -- eval [<k>]   Evaluate KNN accuracy on 20% hold-out (default k=3)
//
// Data files (relative to the working directory):
//   data/train-images-idx3-ubyte   — 60 000 × 28 × 28 grayscale pixel arrays
//   data/train-labels-idx1-ubyte   — 60 000 digit labels (0–9)
//
// Run `make download` once to fetch and decompress these files before use.
//
mod hermes;

use rayon::prelude::*;
use std::fs::File;
use std::io::{Read, Seek, SeekFrom};

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

fn load_image(path: &str, index: usize) -> std::io::Result<Vec<u8>> {
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
    let mut pixels = vec![0u8; ROWS * COLS];
    f.read_exact(&mut pixels)?;
    Ok(pixels)
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

/// Load all N images from an IDX3 file into a flat Vec<u8> (N * 784 bytes).
fn load_all_images(path: &str) -> std::io::Result<Vec<[u8; 784]>> {
    let mut f = File::open(path)?;
    let magic = read_u32_be(&mut f)?;
    assert_eq!(magic, 0x0000_0803, "invalid images magic number");
    let num_images = read_u32_be(&mut f)? as usize;
    let rows = read_u32_be(&mut f)? as usize;
    let cols = read_u32_be(&mut f)? as usize;
    assert_eq!(rows, ROWS);
    assert_eq!(cols, COLS);
    // Header is 16 bytes; seek is already past it after four u32 reads.
    let mut raw = vec![0u8; num_images * ROWS * COLS];
    f.read_exact(&mut raw)?;
    let images: Vec<[u8; 784]> = raw
        .chunks_exact(784)
        .map(|chunk| {
            let mut arr = [0u8; 784];
            arr.copy_from_slice(chunk);
            arr
        })
        .collect();
    Ok(images)
}

/// Load all N labels from an IDX1 file.
fn load_all_labels(path: &str) -> std::io::Result<Vec<u8>> {
    let mut f = File::open(path)?;
    let magic = read_u32_be(&mut f)?;
    assert_eq!(magic, 0x0000_0801, "invalid labels magic number");
    let num_labels = read_u32_be(&mut f)? as usize;
    // Header is 8 bytes; already past it.
    let mut labels = vec![0u8; num_labels];
    f.read_exact(&mut labels)?;
    Ok(labels)
}

fn render(pixels: &[u8]) {
    for row in 0..ROWS {
        let line: String = pixels[row * COLS..(row + 1) * COLS]
            .iter()
            .flat_map(|&p| {
                let idx = (p as usize) * (PALETTE.len() - 1) / 255;
                let ch = PALETTE[idx];
                [ch, ch]
            })
            .collect();
        println!("{}", line);
    }
}

fn main() {
    let images_path = "data/train-images-idx3-ubyte";
    let labels_path = "data/train-labels-idx1-ubyte";

    let mut args = std::env::args().skip(1);
    let subcommand = args.next().unwrap_or_else(|| "view".to_string());

    match subcommand.as_str() {
        "view" => {
            let index: usize = args
                .next()
                .and_then(|s| s.parse().ok())
                .unwrap_or(0);

            let label = load_label(labels_path, index).expect("failed to load label");
            let pixels = load_image(images_path, index).expect("failed to load image");

            println!("Label: {label}  (sample index {index})");
            render(&pixels);
        }
        "eval" => {
            let k: usize = args
                .next()
                .and_then(|s| s.parse().ok())
                .unwrap_or(3);

            const TRAIN_END: usize = 48_000;
            const TOTAL: usize = 60_000;
            const TEST_COUNT: usize = TOTAL - TRAIN_END;

            println!("Loading all {TOTAL} images and labels...");
            let images = load_all_images(images_path).expect("failed to load images");
            let labels = load_all_labels(labels_path).expect("failed to load labels");

            println!("Fitting KNN (k={k}) on {TRAIN_END} training samples...");
            let mut clf = hermes::KnnClassifier::new(k);
            clf.fit(&images[..TRAIN_END], &labels[..TRAIN_END]);

            println!("Evaluating on {TEST_COUNT} test samples...");
            // Predict all test samples in parallel; each call is read-only on clf.
            let results: Vec<(u8, u8)> = (TRAIN_END..TOTAL)
                .into_par_iter()
                .map(|i| (clf.predict(&images[i]), labels[i]))
                .collect();

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
                "\nAccuracy: {correct}/{TEST_COUNT} ({:.2}%)",
                correct as f64 / TEST_COUNT as f64 * 100.0
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
        other => {
            eprintln!("Unknown subcommand: {other:?}");
            eprintln!("Usage:");
            eprintln!("  cargo run -- view [<index>]");
            eprintln!("  cargo run --release -- eval [<k>]");
            std::process::exit(1);
        }
    }
}
