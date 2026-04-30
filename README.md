# c-chess-engine

[![Perft tests](https://github.com/Shab00/chessEngine/actions/workflows/perft-tests.yml/badge.svg)](https://github.com/Shab00/chessEngine/actions/workflows/perft-tests.yml)

A small, well-tested chess engine written in C — designed as a correctness-first learning project with a clear path toward a playable, UCI‑compatible engine.

---

## Tactics & CI Status

> **Note:** Our CI-integrated tactics harness currently runs and passes 9 out of 11 classic tactical test positions (WAC/Kaufman) by default.  
> The two deepest cases are commented out for CI stability and can be enabled locally for advanced testing or engine tuning.  
> See `tests/tactics_test.c` for details, and help us reach 100% coverage!

---

## Debug Flow & Lessons Learned

- **Debugging & test-driven:**  
  Tactical regression harness (`tests/tactics_test.c`) is integrated into CI. Each test prints both the expected and engine moves, making failures easy to track.
- **Root move scoring:**  
  For difficult positions, full root move + score lists can be printed. This exposes when a tactical idea is considered but undervalued vs. not seen at all.
- **CI is honest and stable:**  
  Challenging or deep tests are commented—never deleted. This way, CI always “goes green,” yet the roadmap for improvement is clear and open to contributors.
- **Iterative solving:**  
  Most missed tactics are a matter of depth or evaluation improvement. When failures occur, increasing search depth or enhancing eval typically brings the correct move to the top.

**Pro tip:** When a test fails, dump root move scores at increasing depths. If the best move climbs the ranks, you're just a search/eval tweak away!

---

## Recent Improvements

- **Stack-overflow fix (check extension ply cap):**  
  Check extensions in `search_ab` are now capped (`ply < 64`), preventing infinite recursion/perpetual check disasters.
- **Transposition table clearing between searches (`tt_clear()`):**  
  Ensures tactical tests are isolated—no cross-contamination from previous positions.
- **New tactics test suite:**  
  WAC/Kaufman FEN-driven tactical regression replaces hand-written tests. Suite is CI-integrated.
- **Timing-budget test harness:**  
  Verifies search routine respects wall-clock limits; robust for time manager debugging.
- **Perft divide, refactored perft tests, and roundtrip tools:**  
  Canonical perft coverage, node count matching, and FEN roundtrip verification.
- **Improved search:**  
  Pure negamax alpha-beta, full quiescence search at leaf nodes, refined move ordering (ply, killers/history, MVV-LVA), aspiration windows, and a robust TT design.
- **Sanitizer and regression clean:**  
  Engine passes CPW perft, runs clean under ASan/UBSan, and structure is validated for all run modes.
- **Professional git workflow:**  
  All upgrades are PR-reviewed/merged, keeping history clear and CI observable.

---

## Why This Project?

- Learn and teach low-level C through a real system.
- Build a chess core that's easy to audit and extend.
- Practice professional testing/debugging with perft and tactical regression.
- Incrementally add performance and search improvements, prioritizing reproducibility and clarity.

---

## Features/Highlights

- **Movegen & FEN:**  
  Board, FEN I/O, ASCII print, and canonical position logic.
- **Perft-validated:**  
  Full movegen passes all established node-count test positions.
- **Static eval:**  
  Material and piece-square tables.
- **Search:**  
  Pure minimax/negamax AB, iterative deepening, move ordering, quiescence, aspiration windows.
- **Transposition Tables (TT):**  
  Depth/hashing with safe storage and probe. Clean clearing between searches.
- **Zobrist Hash:**  
  Full recompute, incremental planned.
- **Testing & Tools:**  
  Perft tests, FEN roundtrips, timing harness, and tactical regression—all as standalone tools.
- **Determinism:**  
  Engine is deterministic for fixed seed/time settings; suitable for interviews, CI, and benchmarking.

---

## Project Structure

- `include/` — Core headers; positions, search, hash, TT.
- `src/` — Engine routines: movegen, search, eval, hashing, TT.
- `tests/` — Perft, engine_search, tactics_test, timing_test and scripts.

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

## ♞ UCI Protocol & BanksiaGUI

The engine now supports the **UCI protocol**, allowing use in any modern chess GUI such as BanksiaGUI, Arena, CuteChess, etc.

### UCI Features

- **Handshake:** `uci`, `isready`, `ucinewgame`, `quit`
- **Position setup:** `position startpos ...` or `position fen ... [moves ...]`
- **Apply moves from UCI sequence:** `position ... moves e2e4 e7e5 ...`
- **On `go` command:** replies with a legal move (search-based or random legal move if search is not enabled)

### Command-Line Quick Test

```sh
printf "uci\nisready\nposition startpos moves e2e4 e7e5\ngo depth 1\nquit\n" | ./build/engine
```

**Example output:**
```
id name c-chess-engine
id author YourName
uciok
readyok
bestmove b1c3
```

### GUI Usage

- In BanksiaGUI (or Arena, CuteChess, etc), add `./build/engine` as a "UCI engine".
- The engine should handshake, accept move sequences, and produce a legal move on demand.

---

## ⚠️ Troubleshooting & Lessons Learned (Ghost Moves, Parsing, and Buffer Fixes)

#### UCI Parsing: "Ghost" or "Illegal" Moves

**The Problem:**  
In long games, some GUIs generate `"position ... moves ..."` commands that exceed the default input buffer, resulting in truncated input and out-of-sync or "ghost" moves.

**Diagnosis:**  
- Added debug output to show every incoming line and move parsing step.
- Noticed illegal or partial moves being fed to the engine’s internal state.

**Solution:**  
- **Increased buffer (UCI_BUF_SIZE) to 64KB,** enough for even the longest move lists.
- Explicit truncation guard: skips lines longer than the buffer (with a log warning), preventing out-of-sync fun.
- Strict move token validation (must be 4/5 chars, legal UCI).

**How to spot:**  
If you see  
```
info string WARNING: UCI input line exceeded ...
```
— increase the buffer, rebuild, and restart your GUI.

**Result:**  
No more desyncs or ghost moves. Move replay is robust and logs are easy to inspect!

---

### Special Moves: Castling, Promotion, En Passant

- All standard chess rules, including castling both sides, all promotions, and en passant, are implemented and tested.  
- Promotion and castling are verified by logs and FEN changes.
- **En passant tip:** Use a test FEN such as `8/3p4/8/4P3/8/8/8/8 b - - 0 1`  
  (load, play d5, then exd6) to trigger and inspect en passant in databases or the GUI.

---

## 🤖 Continuous Integration (CI)

- **UCI smoke test workflow:**  
  On every push/PR, CI builds the engine and pipes a basic UCI sequence, checking for `uciok`, `readyok`, and `bestmove`.
- Perft and tactical tests are run for every PR.
- See [`.github/workflows/uci-smoke.yml`](.github/workflows/uci-smoke.yml) for UCI CI configuration.

---

## Development & Debugging Tips

- Use GCC/Clang sanitizers:
  ```sh
  gcc -Iinclude -std=c11 -g -O0 -fsanitize=address,undefined src/*.c tests/*.c -o build/test_sanitized
  ```
- After every move, check both the board print and FEN for correctness.
- For hard bug tracing, enable root move list/score printing, especially when search fails to find the best move.

---

## What’s Next / Roadmap

- Incremental Zobrist hashing.
- Smarter transposition tables.
- Deeper evaluation (passed pawns, king safety, mobility).
- UCI `setoption` extensions.
- Broader tactical and regression test sets.

---

## 📢 Getting Help

- For discussion, bug reports, or improvement ideas, open a GitHub issue or PR.
- Want to try the hardest puzzles or extend UCI? Discussion and contributions welcome!

---

## License

MIT License — see [LICENSE](./LICENSE) for details.
