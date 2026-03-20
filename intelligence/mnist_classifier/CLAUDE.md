# MNIST Classifier — project guide

Brute-force KNN and two-layer MLP classifiers for the MNIST handwritten digit
dataset, with an ASCII terminal viewer.  C implementation using a vtable-based
classifier interface (`fit` / `predict`).

## Module layout

```
src/
  main.rs        — Rust CLI (unchanged)
  hermes.rs      — Rust classifiers (unchanged)
error.h/error.c  — forge_ err_stack infrastructure
par.h/par.c      — forge_par_map (pthreads parallel map)
hermes.h/hermes.c — Classifier vtable, KNN, MLP (C port)
main.c           — C CLI entry point
.build/          — MNIST data files + compiled objects + binary
Makefile         — download, compile, view, knn, nn, clean, fmt targets
CLAUDE.md        — this file
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

Pixels are normalised to `float` in `[0.0, 1.0]` at load time (`/ 255.0`).

## Train / test split

The official MNIST split is used directly — no manual holdout needed:

- **Fit** on all 60 000 training samples
- **Evaluate** on the separate 10 000 test samples

## Commands

| Makefile target | Description |
|---|---|
| `make download` | Fetch + decompress MNIST files |
| `make compile`  | Build `.build/mnist` (all C objects + link) |
| `make view`     | Render training sample 0 (ASCII art) |
| `make knn`      | KNN accuracy benchmark (default k=5) |
| `make nn`       | MLP accuracy benchmark (default hidden=128) |
| `make clean`    | Delete `.build/` |
| `make fmt`      | clang-format all C sources |

### `view` subcommand

```
.build/mnist view [<index>]
```

Renders one training sample as ASCII art (default index 0, range 0–59999).

### `knn` subcommand

```
.build/mnist knn [<k>]
```

Fits KnnClassifier on all 60 000 training samples, evaluates on 10 000 test
samples.  Default k=5.  Expected accuracy: ~97%.

### `nn` subcommand

```
.build/mnist nn [hidden] [lr] [epochs] [batch]
```

Trains NeuralNetClassifier (784 → hidden → 10) on all 60 000 training samples,
evaluates on 10 000 test samples.
Defaults: hidden=128, lr=0.1, epochs=10, batch=64.
Expected accuracy: ~97–98% with defaults.

---

## Adding a new subcommand — checklist

When adding a new classifier or CLI subcommand, update **all four** of these:

1. **`hermes.h` / `hermes.c`** — add struct embedding `struct hermes_classifier ops`
   as first member; implement `fit` and `predict` function pointers; add
   `hermes_<name>_init` / `hermes_<name>_deinit` declarations.

2. **`main.c`**
   - Add a new `else if (strcmp(sub, "subcommand") == 0)` branch in `main`.
   - Add the subcommand to the usage comment at the top of the file.
   - Add the subcommand to `print_usage()`.

3. **`Makefile`**
   - Add the target name to the `.PHONY` line.
   - Add a `target: $(BUILD)/mnist download` rule that invokes `$(BUILD)/mnist <subcommand>`.

4. **`CLAUDE.md`** (this file)
   - Add a row to the Commands table.
   - Add a dedicated subsection under "Commands" describing arguments and defaults.
   - Update the Module layout section if new source files were added.
