# MNIST Classifier — project guide

Brute-force KNN and two-layer MLP classifiers for the MNIST handwritten digit
dataset, with an ASCII terminal viewer.  Go implementation using a `Classifier`
interface (`Fit` / `Predict`).

## Module layout

```
cmd/
  main.go          — CLI entry point (view / knn / nn subcommands)
src/
  mnist.go         — IDX binary loading and ASCII image rendering
  knn.go           — Classifier interface + KNNClassifier
  nn.go            — NNClassifier (MLP forward/backward + SGD)
  eval.go          — Parallel evaluation and per-class accuracy
go.mod             — module classifier, go 1.22
Makefile           — download, compile, view, knn, nn, clean targets
CLAUDE.md          — this file
.build/            — MNIST data files + compiled binary
```

## Data requirements

Run `make download` once before any other command.  Downloads and decompresses
four files into `.build/`:

| File | Samples | Description |
|------|---------|-------------|
| `train-images-idx3-ubyte` | 60 000 | Training pixel arrays (28×28, IDX3) |
| `train-labels-idx1-ubyte` | 60 000 | Training digit labels 0–9 (IDX1) |
| `t10k-images-idx3-ubyte`  | 10 000 | Test pixel arrays (28×28, IDX3) |
| `t10k-labels-idx1-ubyte`  | 10 000 | Test digit labels 0–9 (IDX1) |

Pixels are normalised to `float32` in `[0.0, 1.0]` at load time (`/ 255.0`).

## Commands

| Makefile target | Description |
|---|---|
| `make download` | Fetch + decompress MNIST files |
| `make compile`  | Build `.build/classifier` |
| `make view`     | Render training sample 0 (ASCII art) |
| `make knn`      | KNN accuracy benchmark (default k=5) |
| `make nn`       | MLP accuracy benchmark (default hidden=128) |
| `make clean`    | Delete `.build/` |

### `view` subcommand

```
classifier view [<index>]
```

Renders one training sample as ASCII art (default index 0, range 0–59999).

### `knn` subcommand

```
classifier knn [-k <int>]
```

Fits KNNClassifier on all 60 000 training samples, evaluates on 10 000 test
samples.  Default k=5.  Expected accuracy: ~97%.

### `nn` subcommand

```
classifier nn [-hidden <int>] [-lr <float>] [-epochs <int>] [-batch <int>]
```

Trains NNClassifier (784 → hidden → 10) on all 60 000 training samples,
evaluates on 10 000 test samples.
Defaults: hidden=128, lr=0.1, epochs=10, batch=64.
Expected accuracy: ~97–98% with defaults.

## Adding a new subcommand — checklist

1. **`src/`** — add a new file implementing `Classifier` (Fit + Predict).
2. **`cmd/main.go`** — add a `case` in the `switch` and a `runXxx` function.
3. **`Makefile`** — add the target to `.PHONY` and add a recipe.
4. **`CLAUDE.md`** — update the Commands table and add a subsection.
