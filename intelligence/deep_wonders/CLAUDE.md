# deep_wonders

AlphaZero-style reinforcement learning for 7 Wonders Duel.

## Build

```
make            # Build all binaries
make test       # Run unit tests
make clean      # Clean build artifacts
```

## Binaries

- `.build/debug/play` — Human vs AI interactive play
- `.build/debug/self_play` — AI vs AI self-play, generates training data
- `.build/debug/train` — Training (stub)

## Architecture

- `src/game.rs` — Game state, rules, legal actions (wonder drafting: 12 cards, snake draft)
- `src/mcts.rs` — Monte Carlo Tree Search
- `src/nn.rs` — Neural network (policy + value heads, stub: uniform policy)
- `src/log.rs` — Logging utilities
- `src/bin/play.rs` — Human vs AI entry point
- `src/bin/self_play.rs` — Self-play entry point
- `src/bin/train.rs` — Training entry point

## Conventions

- Rust 2024 edition
- `snake_case` for functions/variables, `CamelCase` for types
- No `unsafe`, no `unwrap` in library code
- Tests live inline in each module via `#[cfg(test)]`

## Coding Style

- Simple structs with methods; no traits beyond standard derives
- Avoid heavy generics; prefer concrete types
- Error handling: return `bool` or `Option`/`Result` as appropriate
- Raw arrays preferred over `Vec` for fixed-size data
