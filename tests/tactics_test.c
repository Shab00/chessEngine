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
    {"3k4/8/8/8/8/8/8/3QK3 w - - 0 1",
     SQ('d','1'), SQ('d','8'), 0, "Simple mate in 1: Qd1-d8#"},

    {"7k/5KQ1/8/8/8/8/8/8 w - - 0 1",
     SQ('g','7'), SQ('h','7'), 0, "Simple mate in 1: Qg7-h7#"},

    {"8/5k1p/1p1pRp2/p2P4/P1P3Pp/1P4bP/6K1/8 w - - 0 49",
     SQ('c','4'), SQ('c','5'), 0, "White plays c4-c5"},

    {"1rbq1rk1/p1b1nppp/1p2p3/8/1B1pN3/P2B4/1P3PPP/2RQ1R1K w - - 0 1",
     SQ('e','4'), SQ('f','6'), 0, "bm Nf6+"},

    {"3r2k1/p2r1p1p/1p2p1p1/q4n2/3P4/PQ5P/1P1RNPP1/3R2K1 b - - 0 1",
     SQ('f','5'), SQ('d','4'), 0, "bm Nxd4"},
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

        int depth = 6;
        SearchContext ctx;
        search_context_init(&ctx, depth, 10000);
        tt_stats_reset();

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
