// MNIST terminal viewer
//
// hermes:v1
//
// Reads a single sample from the MNIST training set and renders it as ASCII art.
//
// Usage:
//   cargo run [-- <index>]
//
//   <index>   Zero-based sample index into the training set (default: 0).
//             The training set contains 60 000 samples (indices 0–59999).
//
// Data files (relative to the working directory):
//   data/train-images-idx3-ubyte   — 60 000 × 28 × 28 grayscale pixel arrays
//   data/train-labels-idx1-ubyte   — 60 000 digit labels (0–9)
//
// Run `make download` once to fetch and decompress these files before use.
//
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
    let index: usize = std::env::args()
        .nth(1)
        .and_then(|s| s.parse().ok())
        .unwrap_or(0);

    let images_path = "data/train-images-idx3-ubyte";
    let labels_path = "data/train-labels-idx1-ubyte";

    let label = load_label(labels_path, index).expect("failed to load label");
    let pixels = load_image(images_path, index).expect("failed to load image");

    println!("Label: {label}  (sample index {index})");
    render(&pixels);
}
