#include <stdio.h>
#include <string.h>
#include "movegen.h"
#include "position.h"
#include "search.h"
#include "search_context.h"
#include "tt.h"
#include "hash.h"
#include "print.h"

typedef struct {
    const char *fen;
    int expected_from, expected_to, expected_promo;
    const char *desc;
} TacticTest;

/* Square index helper: file='a'..'h', rank='1'..'8' (both as char literals) */
#define SQ(f, r)  (((r) - '1') * 8 + ((f) - 'a'))

TacticTest tests[] = {
    // WAC.001: Qg3-g6
    {"2rr3k/pp3pp1/1nnqbN1p/3pN3/2pP4/2P3Q1/PPB4P/R4RK1 w - - 0 1",
     SQ('g','3'), SQ('g','6'), 0, "WAC.001 bm Qg6"},

    // WAC.002: Rb3xb2
    {"8/7p/5k2/5p2/p1p2P2/Pr1pPK2/1P1R3P/8 b - - 0 1",
     SQ('b','3'), SQ('b','2'), 0, "WAC.002 bm Rxb2"},

    // WAC.003: Re3-g3
    {"5rk1/1ppb3p/p1pb4/6q1/3P1p1r/2P1R2P/PP1BQ1P1/5RKN w - - 0 1",
     SQ('e','3'), SQ('g','3'), 0, "WAC.003 bm Rg3"},

    // WAC.004: Qh6xh7+
    {"r1bq2rk/pp3pbp/2p1p1pQ/7P/3P4/2PB1N2/PP3PPR/2KR4 w - - 0 1",
     SQ('h','6'), SQ('h','7'), 0, "WAC.004 bm Qxh7+"},

    // WAC.005: Qc6-c4+
    {"5k2/6pp/p1qN4/1p1p4/3P4/2PKP2Q/PP3r2/3R4 b - - 0 1",
     SQ('c','6'), SQ('c','4'), 0, "WAC.005 bm Qc4+"},

    // WAC.006: Rb6-b7
    {"7k/p7/1R5K/6r1/6p1/6P1/8/8 w - - 0 1",
     SQ('b','6'), SQ('b','7'), 0, "WAC.006 bm Rb7"},

    // WAC.007: Ng4-e3
    {"rnbqkb1r/pppp1ppp/8/4P3/6n1/7P/PPPNPPP1/R1BQKBNR b KQkq - 0 1",
     SQ('g','4'), SQ('e','3'), 0, "WAC.007 bm Ne3"},

    // WAC.008: Re7-f7
    {"r4q1k/p2bR1rp/2p2Q1N/5p2/5p2/2P5/PP3PPP/R5K1 w - - 0 1",
     SQ('e','7'), SQ('f','7'), 0, "WAC.008 bm Rf7"},

    // WAC.009: Bd6-h2+
    {"3q1rk1/p4pp1/2pb3p/3p4/6Pr/1PNQ4/P1PB1PP1/4RRK1 b - - 0 1",
     SQ('d','6'), SQ('h','2'), 0, "WAC.009 bm Bh2+"},
};

const int NUM_TESTS = sizeof(tests) / sizeof(tests[0]);

void square_to_alg(int sq, char out[3]) {
    out[0] = 'a' + (sq % 8);
    out[1] = '1' + (sq / 8);
    out[2] = '\0';
}

int main(void) {
    /* Initialize zobrist keys BEFORE any FEN parsing or hashing */
    zobrist_init(0);

    /* Initialize transposition table */
    tt_init(32);

    int pass_count = 0;
    for (int i = 0; i < NUM_TESTS; ++i) {
        Position pos;
        char err[256];
        if (position_from_fen(&pos, tests[i].fen, err, sizeof(err)) != POS_OK) {
            printf("Test %d: FEN error: %s\n", i + 1, err);
            continue;
        }

        tt_clear();       /* flush all stale TT entries from previous test */
        tt_stats_reset();

        int depth = 6;
        SearchContext ctx;
        search_context_init(&ctx, depth, 10000);

        int found_from = -1, found_to = -1, found_promo = 0;
        search_root(&pos, depth, &found_from, &found_to, &found_promo, &ctx);

        char exp_from_s[3], exp_to_s[3], got_from_s[3], got_to_s[3];
        square_to_alg(tests[i].expected_from, exp_from_s);
        square_to_alg(tests[i].expected_to, exp_to_s);

        printf("Test %2d: %s\n", i + 1, tests[i].desc);
        printf("  Expected: %s%s\n", exp_from_s, exp_to_s);

        if (found_from >= 0) {
            square_to_alg(found_from, got_from_s);
            square_to_alg(found_to, got_to_s);
            printf("  Engine:   %s%s\n", got_from_s, got_to_s);
        } else {
            printf("  Engine:   (no move found)\n");
        }

        int pass = (found_from == tests[i].expected_from &&
                    found_to == tests[i].expected_to);
        printf("  Result: %s\n\n", pass ? "PASS" : "FAIL");
        if (pass) pass_count++;
    }
    printf("=== Tactics tests: %d/%d passed ===\n", pass_count, NUM_TESTS);

    tt_free();
    return (pass_count == NUM_TESTS ? 0 : 1);
}
