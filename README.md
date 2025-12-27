# c-chess-engine

A small chess engine written in C.

Goals
- Learn C deeply while building a working chess engine.
- Produce a UCI‑compatible engine that can play online (via a server or chess platform).
- Prioritize correctness and tests (perft), then improve strength and performance.

Project overview
This repository implements a simple, well‑tested chess engine prototype in C. The focus is correctness, reproducible tests, and incremental development. The code is intentionally straightforward so it is easy to audit and extend.

What’s included (current)
- Board representation (Position struct)
- Full FEN parsing (FEN → Position)
- FEN serializer (Position → FEN)
- ASCII board printer
- Automated round‑trip FEN tests (parse → serialize → re‑parse → compare)
- Basic validation of positions (e.g. one king per side)

Milestones
1. Board representation, move generation, perft tests. (current focus)
2. Search: minimax → alpha‑beta, iterative deepening, time management.
3. Transposition table, enhancements, performance improvements.
4. UCI protocol implementation.
5. Server integration / online deployment.

Phase 1 acceptance criteria
- Correct, unambiguous position representation.
- FEN parsing and serialization implemented.
- ASCII/CLI board printer for debugging.
- FEN round‑trip tests (parse → serialize → re‑parse → compare) passing.
- Tests run cleanly under sanitizers (AddressSanitizer / UndefinedBehaviorSanitizer) where supported.

Quick start (build & test)
These commands assume you are in the project root.

1) Create the build directory
mkdir -p build

2) Compile the core files
gcc -Iinclude -std=c11 -Wall -Wextra -c src/position.c -o build/position.o
gcc -Iinclude -std=c11 -Wall -Wextra -c src/position_fen.c -o build/position_fen.o

3) Compile the round‑trip test
gcc -Iinclude -std=c11 -Wall -Wextra -c tests/fen_roundtrip.c -o build/fen_roundtrip.o

4) Link the test binary
gcc build/fen_roundtrip.o build/position.o build/position_fen.o -o build/fen_roundtrip

5) Run a single check (example)
./build/fen_roundtrip "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

6) Run the automated test suite
Make the runner executable (once):
chmod +x tests/run_fen_tests.sh

Then:
./tests/run_fen_tests.sh

Sanitizers (recommended during development)
Build and run with AddressSanitizer and UndefinedBehaviorSanitizer to catch memory errors and UB:
gcc -Iinclude -std=c11 -g -O0 -fsanitize=address,undefined -fno-omit-frame-pointer \
  src/position.c src/position_fen.c tests/fen_roundtrip.c -o build/fen_roundtrip_sanitized

Run:
./build/fen_roundtrip_sanitized "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

Notes:
- On macOS you may see allocator messages from the sanitizer toolchain; these are platform-specific. If you get ASan/UBSan reports, paste them to debug.
- The serializer writes castling rights in canonical order `KQkq`. Validation (position_validate) enforces basic invariants, including exactly one king per side.

Tests and test data
- tests/fen_tests.txt — list of canonical and tricky FENs used by the runner.
- tests/run_fen_tests.sh — runs each line in the FEN file through the round‑trip test.
- tests/fen_roundtrip.c — test program that performs parse → serialize → parse and compares Positions.

Development workflow suggestions
- Use sanitizers when changing low-level code.
- Add perft tests once move generation is implemented; store expected node counts to detect regressions.
- Add CI (GitHub Actions) to run tests on push/PR. A sample workflow is provided in the repo (or can be added).

Contributing
- Open issues for bugs / desired features.
- Create a topic branch for any non‑trivial work, add tests, and open a PR for review.
- Keep changes small and well tested.

Next steps
- Implement move generation and perft tests (Phase 1 → Phase 2).
- Implement position_to_fen if missing (should already be present in src/position_fen.c).
- Add Makefile or simple build scripts for convenience.
- Add CI to run tests and sanitizers automatically.

Contact / Notes
This project is intended as a learning vehicle and a foundation for building a UCI‑compatible engine. If you want, I can:
- Add a Makefile with handy targets (build, test, sanitize).
- Add the GitHub Actions workflow file for CI.
- Start Phase 2 planning (move generation API, move representation, and perft test cases).
