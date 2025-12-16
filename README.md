# c-chess-engine

Project: a chess engine written in C.

Goals:
- Learn C deeply while building a working chess engine.
- Produce a UCI-compatible engine that can play online (via a server or chess platform).
- Prioritize correctness and tests (perft), then improve strength and performance.

Milestones:
1. Board representation, move generation, perft tests.
2. Search (minimax → alpha-beta), evaluation.
3. Transposition table, iterative deepening, time management.
4. UCI protocol implementation.
5. Server integration / online deployment.

Development notes:
- Start with simple board (array or 0x88), then consider bitboards.
- Use AddressSanitizer and UndefinedBehaviorSanitizer during development.
- Automate perft and regression tests in Makefile.

How to start:
- Implement board.c / board.h with FEN load, move generation and perft.
- Make perft tests to match reference node counts.
