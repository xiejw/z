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

Run `make download` once before any other command.

- `data/train-images-idx3-ubyte` — 60 000 × 28 × 28 pixel arrays (IDX3 format)
- `data/train-labels-idx1-ubyte` — 60 000 digit labels 0–9 (IDX1 format)

## 80/20 split

- Train: indices `0..48_000`
- Test:  indices `48_000..60_000` (12 000 samples)

## Commands

| Makefile target | cargo equivalent | Description |
|---|---|---|
| `make download` | — | Fetch + decompress MNIST files |
| `make run` | `cargo run -- view 0` | ASCII-render sample 0 |
| `make knn` | `cargo run --release -- knn [k]` | KNN benchmark (default k=3) |
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

Fits KnnClassifier on 48 000 samples, evaluates on 12 000.  Default k=3.
Expected accuracy: ~96–97%.

### `nn` subcommand

```
cargo run --release -- nn [hidden] [lr] [epochs] [batch]
```

Trains NeuralNetClassifier (784 → hidden → 10) on 48 000 samples, evaluates
on 12 000.  Defaults: hidden=128, lr=0.1, epochs=10, batch=64.
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
