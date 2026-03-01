# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This repository contains implementations of algorithms from **Donald Knuth's "The Art of Computer Programming" (TAOCP), Volume 4B**. All code is **written by hand** to follow exactly the algorithms as described in TAOCP — preserving the step labels (B1, B2, ...), goto-based control flow, and 1-based array indexing as Knuth specifies them. The goal is pedagogical fidelity to the source material, not idiomatic modern C++.

Algorithms implemented:
- **Boolean Basics**: Horn Functions (V4 7.1.1C), 2-SAT via Strongly Connected Components (V4 7.1.1 Theorem K)
- **Backtracking**: Basic Backtrack (V4 7.2.2B), Walker's Backtrack (V4 7.2.2W)
- **Exact Cover**: Dancing Links / Algorithm X (V4 7.2.2.1X)
- **Satisfiability**: SAT by Watching (V4 7.2.2.2B)
- **Graph Algorithms**: Strong Components (V4 7.4.1.2T), All Hamiltonian Cycles (V4 7.2.2.4H)

## Commands

### Build and Test (from project root)

```bash
make compile       # Compile all modules
make test          # Run all tests
make ASAN=1 test   # Run all tests with Address Sanitizer
make release       # Build all modules with -O3 -march=native -flto -ffast-math
make fmt           # Format all C++ code
make clean         # Remove all build artifacts
```

### Per-module (from within any module directory)

```bash
make compile            # Compile this module
make test               # Run this module's tests
make RELEASE=1 compile  # Optimized build
make ASAN=1 test        # Test with Address Sanitizer
```

### Notes / LaTeX documentation

```bash
cd notes && make run    # Build PDF documentation
```

## Architecture

Each algorithm lives in its own directory named `v{volume}_{section}.algo_{letter}_{description}/`. Modules are self-contained and build independently.

**Source layout within each module:**
- `cmd/main.cc` — algorithm implementation (the core TAOCP translation)
- `cmd/*_test.cc` — additional test binaries (when present)
- `src/*.cc`, `src/*.h` — shared utilities: logging (`log.h/cc`), graph structures (`graph_sgb.cc`), data structures

**Build templates** (in `mk/`):
- `Makefile.v1` — single binary per module
- `Makefile.v2` — supports multiple test binaries via `TEST_template` and `CMD_template` macros

All binaries output to `.build/` within each module directory.

## Code Style Conventions

- **Goto-based state machines**: Steps like `B1:`, `B2:` etc. directly mirror TAOCP pseudocode labels. This is intentional — do not refactor gotos away.
- **1-based arrays**: Arrays are allocated with one extra slot and indexed from 1, matching TAOCP notation.
- **MEMs metric**: Performance is measured in memory accesses (Knuth's MEMs). Instrumented via `mem_access_counter`.
- **`forge:` comments**: Mark files for checksum checks..
- **C++17** with `-Wall -Werror -pedantic -Wextra -fno-rtti -fno-exceptions`.
- Multi-language implementations exist for some modules (Go in `go/`, Rust in `rs/`).
