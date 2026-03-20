# c-chess-engine

## Recent improvements

- **Quiescence search:** Added and called at leaf nodes for more accurate search extensions and leaf evaluation. Thoroughly tested with suite.
- **Move ordering using real ply, killer/history indexing & MVV-LVA:** Refactored move scoring to use ply, implemented multi-killer slots, and ensured proper indexing for killer/history tables; improved pruning and search reliability.
- **Aspiration windows:** Integrated aspiration windows into iterative deepening for faster pruning and better root move selection.
- **Perft divide & in-depth perft debugging:** Added a perft divide harness to compare root move node counts against reference results. Used this to debug and fix castling, en-passant, and underpromotion handling.
- **Refactored perft tests:** Updated canonical FENs and reference node counts to match established suite. Perft harness and shell scripts print clear PASS/FAIL and help diagnose errors quickly.
- **FEN roundtrip tester:** Created a dedicated utility and suite to ensure all FEN strings can be parsed and serialized with no fidelity loss.
- **General harness & codebase cleanup:** Deleted deprecated test files (e.g., `perft_debug.c`) and consolidated test logic for easier CI use and maintenance.

[![Perft tests](https://github.com/Shab00/chessEngine/actions/workflows/perft-tests.yml/badge.svg)](https://github.com/Shab00/chessEngine/actions/workflows/perft-tests.yml)

A small, well-tested chess engine written in C — designed as a correctness-first learning project with a clear path toward a playable, UCI‑compatible engine.

This repository is intended both as a learning vehicle for low-level C and as a small but real engine you can extend and profile. It emphasizes reproducible tests (perft), straightforward code, and incremental improvements to search and evaluation.

Why this project
- Deepen systems/C skills: memory management, deterministic testing, sanitizers.
- Build a correct chess core that’s easy to audit and extend.
- Incrementally add search, evaluation, and performance improvements while keeping strong correctness guardrails (perft testing, CI).

What we have accomplished (highlights)
- Board representation, FEN parsing and serialization, ASCII board printing.
- Fully working move generation validated with perft (multiple canonical test positions).
- Static evaluation: material balance + piece-square tables implemented.
- Search:
  - Minimax / alpha‑beta search implemented and integrated.
  - Iterative deepening shell implemented.
  - Basic move ordering (MVV/LVA and promotion boost) implemented and wired into search.
- Transposition table (TT):
  - Public TT API and a working simple TT implementation added.
  - TT probe/store integrated into root search and alpha‑beta nodes safely using a full recompute Zobrist hash.
- Zobrist hashing:
  - Deterministic Zobrist position hashing (full recompute) implemented and used for TT keys.
  - Plan and scaffolding in place for incremental hash updates in make/unmake.
- Tests and tooling:
  - Perft test harness and smoke tests (start position and Kiwipete).
  - Deterministic search verified (repeated runs produce identical output).
  - CI workflow runs perft tests on pushes/PRs.
- Bug fixes and integration work:
  - Resolved linker/visibility issues (exported move-ordering helper).
  - Ensured perft integrity before/after search runs (node counts match).
  - Added a stub TT so search can be exercised safely while the full TT is improved.

Problems we solved
- Ensuring make/unmake correctness: implemented perft and used perft as a canonical verification for move generation and state restoration.
- Alpha‑beta correctness and move semantics: adapted value handling so search returns consistent values regardless of internal side-to-move toggles.
- Move ordering integration: implemented MVV/LVA ordering and a preferred-move mechanism supplied by iterative deepening to increase alpha‑beta cutoffs.
- TT safety: integrated TT in a conservative way (probe only when safe, store with clear flags) while using full recompute hashing to prevent subtle bugs from incorrect incremental updates.
- Reproducible testing: added deterministic seeding for zobrist tables (configurable) and deterministic CLI behavior for easy CI and interviews.

Project structure (important files)
- include/
  - `position.h` — Position struct and helpers.
  - `search.h` — Search API (search_root, search_iterative_deepening).
  - `hash.h` — Zobrist hashing API.
  - `tt.h` — Transposition table API.
- src/
  - `position.c`, `position_fen.c` — FEN parsing/serialization, position routines.
  - `movegen.c` — Move generation (pseudo-legal + legal filter).
  - `eval.c` — Static evaluator (material + PST).
  - `search.c` — Alpha‑beta search + TT integration.
  - `search_id_additions.c` — Iterative deepening + move ordering (MVV/LVA).
  - `zobrist.c` — Zobrist full-recompute hashing.
  - `tt.c` — Simple transposition table implementation.
- tests/
  - `perft` / `perft` tool — perft harness.
  - `engine_search` — CLI search driver using iterative deepening.
  - smoke tests and perft integrity scripts.

Quick start — build and run
(Assumes you are in the project root and have a Unix-like toolchain.)

1. Prepare build directory
```bash
mkdir -p build
```

2. Build the project
```bash
make
```

3. Run smoke/perft checks
```bash
# start position depth 3
FEN='rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1'
./build/perft "$FEN" 3

# run the engine search (iterative deepening)
./build/engine_search "$FEN" 3

# perft integrity: perft before/after engine_search should match
./build/perft "$FEN" 3
./build/engine_search "$FEN" 3
./build/perft "$FEN" 3
```

4. Determinism check
```bash
./build/engine_search "$FEN" 4 > out1.txt
./build/engine_search "$FEN" 4 > out2.txt
diff out1.txt out2.txt || echo "non-deterministic"
```

Development & debugging tips
- Use sanitizers while developing:
  gcc -Iinclude -std=c11 -g -O0 -fsanitize=address,undefined src/*.c tests/*.c -o build/test_sanitized
- If you enable incremental Zobrist updates later, add a debug-mode assertion:
  assert(pos->hash == position_hash(pos)) after every make/unmake to detect mistakes early.
- If a perft mismatch appears after adding TT/incremental hashing:
  - Disable TT (or set tt_init(0)) and see if perft returns to baseline — isolates the problem.
  - Use the debug hash assertion to find the first move that desynchronizes the incremental hash.

What’s next / roadmap
- Implement incremental Zobrist updates inside make_move/unmake_move (performance critical).
- Improve TT replacement policy (two-slot / age bits) and add TT statistics (hits/misses).
- Add quiescence search and more move ordering (killers/history heuristic).
- Add iterative deepening timed search with a simple time manager.
- Implement UCI protocol so engine can play vs GUI/servers.

Why this is relevant for an embedded development internship
- Low-level C: careful memory management, bit/bitboard-like thinking, and deterministic behavior.
- Testing & debugging: sanitizers, reproducible tests, instrumentation.
- Performance tradeoffs: algorithmic design (search, TT) and micro-optimizations.
- Systems thinking: building reliable components (FEN I/O, make/unmake, TT) that compose into a correct system.

License

This project is released under the MIT License — see the [LICENSE](./LICENSE) file for details.
