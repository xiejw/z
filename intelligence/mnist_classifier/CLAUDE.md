# MNIST Classifier — project guide

Brute-force KNN and two-layer MLP classifiers for the MNIST handwritten digit
dataset, with an ASCII terminal viewer.  All classifiers implement the
`hermes::Classifier` trait (`fit` / `predict`).

## Module layout

```
src/
  main.rs      — CLI entry point, IDX file parsers, ASCII renderer, run_eval helper
  hermes.rs    — Classifier trait, gemm, KnnClassifier, NeuralNetClassifier
data/          — MNIST binary files (created by `make download`)
Makefile       — download, run, knn, nn, clean, fmt targets
CLAUDE.md      — this file
```

## Data requirements

Run `make download` once before any other command.  Downloads and decompresses
four files into `data/`:

| File | Samples | Description |
|------|---------|-------------|
| `train-images-idx3-ubyte` | 60 000 | Training pixel arrays (28×28, IDX3) |
| `train-labels-idx1-ubyte` | 60 000 | Training digit labels 0–9 (IDX1) |
| `t10k-images-idx3-ubyte`  | 10 000 | Test pixel arrays (28×28, IDX3) |
| `t10k-labels-idx1-ubyte`  | 10 000 | Test digit labels 0–9 (IDX1) |

Pixels are normalised to `f32` in `[0.0, 1.0]` at load time (`/ 255.0`).

## Train / test split

The official MNIST split is used directly — no manual holdout needed:

- **Fit** on all 60 000 training samples
- **Evaluate** on the separate 10 000 test samples

## Commands

| Makefile target | cargo equivalent | Description |
|---|---|---|
| `make download` | — | Fetch + decompress all four MNIST files |
| `make run` | `cargo run -- view 0` | ASCII-render training sample 0 |
| `make knn` | `cargo run --release -- knn [k]` | KNN benchmark (default k=5) |
| `make nn` | `cargo run --release -- nn [hidden] [lr] [epochs] [batch]` | MLP benchmark |
| `make clean` | — | Delete `data/` and build cache |
| `make fmt` | `cargo fmt` | Format source |

KNN and MLP subcommands must run with `--release`; both are slow in debug mode.

### `view` subcommand

```
cargo run -- view [<index>]
```

Renders one training sample as ASCII art (default index 0, range 0–59999).

### `knn` subcommand

```
cargo run --release -- knn [<k>]
```

Fits KnnClassifier on all 60 000 training samples, evaluates on 10 000 test
samples.  Default k=5.  Expected accuracy: ~97%.

### `nn` subcommand

```
cargo run --release -- nn [hidden] [lr] [epochs] [batch]
```

Trains NeuralNetClassifier (784 → hidden → 10) on all 60 000 training samples,
evaluates on 10 000 test samples.
Defaults: hidden=128, lr=0.1, epochs=10, batch=64.
Expected accuracy: ~97–98% with defaults.

---

## Adding a new subcommand — checklist

When adding a new classifier or CLI subcommand, update **all four** of these:

1. **`src/hermes.rs`** — implement `Classifier` trait for the new struct.

2. **`src/main.rs`**
   - Add a new `"subcommand" =>` arm in the `match` block.
   - Add the subcommand to the usage comment at the top of the file.
   - Add the subcommand to the `eprintln!` usage block in the `other =>` arm.

3. **`Makefile`**
   - Add the target name to the `.PHONY` line.
   - Add a `target: download` rule that invokes `cargo run --release -- <subcommand>`.

4. **`CLAUDE.md`** (this file)
   - Add a row to the Commands table.
   - Add a dedicated subsection under "Commands" describing arguments and defaults.
   - Update the Module layout section if new source files were added.
