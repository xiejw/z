# KNN MNIST Classifier

Brute-force k-nearest-neighbours classifier for the MNIST handwritten digit dataset,
with an ASCII terminal viewer.

## Module layout

```
src/
  main.rs      — CLI entry point, IDX file parsers, ASCII renderer
  hermes.rs    — KnnClassifier (L1 distance, majority-vote predict)
data/          — MNIST binary files (created by `make download`)
Makefile       — download, run, eval, clean targets
```

## Commands

| Command | Description |
|---------|-------------|
| `make download` | Fetch and decompress MNIST training files into `data/` |
| `make run` | View sample 0 as ASCII art |
| `make eval` | Run KNN accuracy benchmark (k=3, ~96–97% expected) |
| `cargo run -- view <index>` | View any sample index (0–59999) |
| `cargo run --release -- eval <k>` | Eval with custom k |

## Data requirements

Run `make download` once before using any other commands. The data files are:

- `data/train-images-idx3-ubyte` — 60 000 × 28 × 28 pixel arrays (IDX3 format)
- `data/train-labels-idx1-ubyte` — 60 000 digit labels 0–9 (IDX1 format)

## 80/20 split

- Train: indices `0..48_000`
- Test:  indices `48_000..60_000` (12 000 samples)

Always run `eval` with `--release`; brute-force KNN over 48 k × 784 bytes is
very slow in debug mode.
