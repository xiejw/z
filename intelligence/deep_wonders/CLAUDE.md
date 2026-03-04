# deep_wonders

AlphaZero-style reinforcement learning for 7 Wonders Duel.

## Build

```
make            # Build all binaries
make release    # Build optimized
make clean      # Clean build artifacts
```

## Binaries

- `.build/play` — Human vs AI interactive play
- `.build/self_play` — AI vs AI self-play, generates training data
- `.build/train` — Training (stub)

## Architecture

- `src/game.{h,cc}` — Game state, rules, legal actions (wonder drafting: 12 cards, snake draft)
- `src/mcts.{h,cc}` — Monte Carlo Tree Search
- `src/nn.{h,cc}` — Neural network (policy + value heads, stub: uniform policy)
- `src/log.{h,cc}` — Logging utilities (forge library)
- `cmd/play.cc` — Human vs AI entry point
- `cmd/self_play.cc` — Self-play entry point
- `cmd/train.cc` — Training entry point

## Conventions

- Namespace: `deep_wonders`
- `#pragma once`, `Camel case`, C++17
- CXXFLAGS: `-Wall -Werror -pedantic -Wextra -fno-rtti -fno-exceptions`
- Function naming: free functions and class methods use `CamelCase` (e.g., `Game::ApplyAction()`, `Game::IsOver()`)
- Memory: `malloc`/`calloc`/`free`, `assert` on allocation

## Coding Style

This codebase uses a C++11-like style that leans heavily toward C:

- **Raw pointers everywhere.** Pass raw pointers, not references. Move semantics
  and `std::move` are avoided. Ownership is implied. All pointers are not owned.
- **Simple classes/structs.** Classes are fine but must be deadly simple, close
  to C style — simple methods, no inheritance, no virtual methods. Free
  functions (e.g., `game_new()`) are legacy; prefer simple class methods for
  new code.
- **No templates** unless absolutely necessary. Avoid heavy template-based `std`
  classes (e.g., prefer raw arrays over `std::vector`, `printf` over
  `std::cout`/`std::string` streams).
- **No exceptions, no RTTI.** Enforced by `-fno-exceptions -fno-rtti`.
- **Error handling** follows a consistent pattern:
  `bool Foo(args..., std::string *err_msg)` — returns `false` on success,
  `true` on error (with message written to `err_msg`).
