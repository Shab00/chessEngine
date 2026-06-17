# c-chess-engine

[![Perft tests](https://github.com/Shab00/chessEngine/actions/workflows/perft-tests.yml/badge.svg)](https://github.com/Shab00/chessEngine/actions/workflows/perft-tests.yml)

A correctness-first chess engine written in C, built as a low-level learning project with strong testing, clear debugging workflows, and a path toward a stronger playable UCI engine.

---

## Current Status

This project is currently in a **well-tested engine foundation** stage.

It already includes:

- legal move generation and board state handling
- perft-tested correctness
- tactical regression coverage
- alpha-beta search with quiescence
- transposition table support
- UCI protocol support for GUI play and testing
- a companion WebAssembly/browser demo powered by this engine

---

## Why This Project?

- Learn and practice low-level C through a real system.
- Build a chess engine core that is easy to inspect, test, and extend.
- Develop professional debugging habits through perft, tactics, CI, and regression testing.
- Incrementally improve search and evaluation while keeping correctness and reproducibility central.

---

## Related Projects

This engine also powers a companion browser/WebAssembly project:

- **Chess Engine WASM Demo Repo:** [Shab00/chess-engine-wasm](https://github.com/Shab00/chess-engine-wasm)
- **Live Browser Demo:** [shab00.github.io/chess](https://shab00.github.io/chess/)

That project focuses on compiling the engine to WebAssembly and connecting it to a browser-based chessboard UI for local and GitHub Pages play.

---

## Features / Highlights

- **Move generation and FEN support**  
  Board representation, FEN parsing/serialization, ASCII board printing, and canonical position handling.
- **Perft-validated correctness**  
  Move generation passes established node-count test positions.
- **Static evaluation**  
  Material scoring and piece-square tables.
- **Search**  
  Negamax / alpha-beta search, iterative deepening, move ordering, quiescence search, and aspiration windows.
- **Transposition tables**  
  Depth-aware hashing with safe probing and clearing between searches.
- **Zobrist hashing**  
  Full recomputation support, with room for future incremental improvements.
- **Testing and tooling**  
  Perft tests, tactical regression, timing harnesses, and FEN roundtrip verification.
- **Determinism**  
  Suitable for CI, debugging, interviews, and repeatable benchmarking.

---

## Tactics & CI Status

> **Note:** The CI-integrated tactics harness currently runs and passes 9 out of 11 classic tactical test positions (WAC/Kaufman) by default.  
> The two deepest cases are commented out for CI stability and can be enabled locally for advanced testing or engine tuning.  
> See `tests/tactics_test.c` for details.

---

## Debugging Workflow & Lessons Learned

- **Test-driven debugging**  
  Tactical regression (`tests/tactics_test.c`) is integrated into CI, and each test prints both the expected and engine moves to make failures easier to inspect.
- **Root move scoring**  
  Difficult positions can be debugged by printing root move + score lists, which helps distinguish between “not seen” and “seen but undervalued.”
- **Stable CI with visible roadmap**  
  Challenging tests are commented rather than removed, keeping CI reliable while preserving future improvement targets.
- **Iterative improvement loop**  
  Many missed tactics are the result of depth or evaluation limitations, so failures often become useful guidance for the next search or eval upgrade.

When a tactical test fails, printing root move scores at increasing depths is often the fastest way to understand whether the engine is close or missing the idea entirely.

---

## Recent Improvements

- **Stack-overflow fix for check extensions**  
  Check extensions in `search_ab` are capped (`ply < 64`) to prevent runaway recursion in perpetual-check-style positions.
- **Transposition table clearing between searches (`tt_clear()`)**  
  Keeps tactical tests isolated and avoids cross-position contamination.
- **Tactical regression harness**  
  WAC/Kaufman FEN-driven tactical testing replaced hand-written tests and is now CI-integrated.
- **Timing-budget harness**  
  Search timing behavior can be tested directly for time-manager debugging.
- **Perft divide and roundtrip tooling**  
  Supports node-count verification and FEN roundtrip validation.
- **Search improvements**  
  Negamax alpha-beta, full quiescence search, refined move ordering, aspiration windows, and a stronger TT structure.
- **Sanitizer and regression cleanliness**  
  The engine passes CPW perft and runs cleanly under ASan/UBSan in supported workflows.

---

## Project Structure

- `include/` — Core headers for positions, search, hashing, and transposition tables
- `src/` — Engine implementation: move generation, search, eval, hashing, TT, UCI, and helpers
- `tests/` — Perft, tactics, timing, engine search, and supporting test scripts

---

## Quick Start

```bash
# Build everything
mkdir -p build
make engine      # or just 'make', must produce ./build/engine

# Run basic perft (nodecount) test
FEN='rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1'
./build/perft "$FEN" 3

# Run the tactical regression suite
./build/tactics_test

# Run timing and engine search smoke checks
./build/engine_search "$FEN" 3
./build/timing_test
```

---

## UCI Protocol & GUI Support

The engine supports the **UCI protocol**, so it can be used in chess GUIs such as BanksiaGUI, Arena, and CuteChess.

### UCI Features

- **Handshake:** `uci`, `isready`, `ucinewgame`, `quit`
- **Position setup:** `position startpos ...` or `position fen ... [moves ...]`
- **Move replay from UCI sequences:** `position ... moves e2e4 e7e5 ...`
- **Search trigger:** `go` returns a legal move, using search when available

### Command-Line Quick Test

```sh
printf "uci\nisready\nposition startpos moves e2e4 e7e5\ngo depth 1\nquit\n" | ./build/engine
```

**Example output:**
```text
id name c-chess-engine
id author YourName
uciok
readyok
bestmove b1c3
```

### GUI Usage

- In BanksiaGUI, Arena, CuteChess, or similar tools, add `./build/engine` as a UCI engine.
- The engine should handshake, accept move sequences, and return a legal `bestmove`.

---

## Troubleshooting: UCI Parsing, Ghost Moves, and Buffer Fixes

### UCI Parsing: “Ghost” or Illegal Moves

**Problem**  
In long games, some GUIs send `"position ... moves ..."` commands that exceed the default input buffer, which can cause truncated input and desynchronized move replay.

**Diagnosis**
- Added debug output for incoming UCI lines and parsed move tokens
- Observed partial or illegal moves entering internal state during long command sequences

**Solution**
- Increased `UCI_BUF_SIZE` to 64 KB
- Added a truncation guard that skips overlong lines with a warning
- Enforced strict move token validation (4/5 chars, legal UCI move format)

**How to spot it**
If you see:

```text
info string WARNING: UCI input line exceeded ...
```

increase the buffer, rebuild, and restart your GUI.

**Result**  
Move replay is now far more robust and easier to debug.

---

## Special Moves

- Castling on both sides is implemented and tested
- Promotions are implemented and verified through logs and FEN changes
- En passant is implemented and testable through controlled positions

Example en passant test FEN:

```text
8/3p4/8/4P3/8/8/8/8 b - - 0 1
```

From there, play `d5` and then `exd6` to inspect en passant handling.

---

## Continuous Integration

- **UCI smoke test workflow**  
  On every push and pull request, CI builds the engine and checks a basic UCI interaction for `uciok`, `readyok`, and `bestmove`.
- **Perft and tactics coverage**  
  Perft and tactical tests run in CI for regression protection.
- See [`.github/workflows/uci-smoke.yml`](.github/workflows/uci-smoke.yml) for the UCI smoke-test configuration.

---

## Development & Debugging Tips

- Use sanitizers when debugging locally:

  ```sh
  gcc -Iinclude -std=c11 -g -O0 -fsanitize=address,undefined src/*.c tests/*.c -o build/test_sanitized
  ```

- After every suspicious move sequence, verify both board print output and FEN.
- For difficult search bugs, print root move lists and scores before changing evaluation or pruning behavior.

---

## Roadmap

Possible future improvements include:

- incremental Zobrist hashing
- smarter transposition table policies
- deeper evaluation terms such as passed pawns, king safety, and mobility
- additional UCI `setoption` support
- broader tactical and regression coverage

---

## Getting Help

- Open an issue or pull request for bug reports, suggestions, or ideas
- Contributions are welcome, especially around testing, evaluation, and search improvements

---

## License

MIT License — see [LICENSE](./LICENSE) for details.
