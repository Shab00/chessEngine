#include <stdio.h>
#include <string.h>
#include "position.h"
#include "search_context.h"
#include "timing.h"
#include "search.h"

typedef struct {
    const char *fen;
    int expected_from, expected_to, expected_promo;
    const char *desc;
} TacticTest;

TacticTest tests[] = {
    // Qd8#
    {"rnb1kbnr/pppp1ppp/8/4p3/8/4P3/PPPP1PPP/RNBQKBNR w KQkq - 0 3", 3, 59, 0, "Mate in 1: Qd8#"},
    {"7k/5KQ1/8/8/8/8/8/8 w - - 0 1", 61, 7, 0, "Simple mate in 1: Qg7-h7#"},
    // Example pawn mate in 1 (edit/add more as needed)
    //{"8/8/7k/6Q1/8/8/8/7K w - - 0 1", 30, 63, 0, "Mate in 1: Qg5-h6#"},
};

const int NUM_TESTS = sizeof(tests)/sizeof(tests[0]);

void square_to_alg(int sq, char out[3]) {
    out[0] = 'a' + (sq % 8);
    out[1] = '1' + (sq / 8);
    out[2] = '\0';
}

void print_move_uci(int from, int to, int promo) {
    char from_alg[3], to_alg[3];
    square_to_alg(from, from_alg);
    square_to_alg(to, to_alg);
    printf("%s%s", from_alg, to_alg);
    if (promo) {
        char promo_char = '?';
        switch(promo) {
            case 1: promo_char = 'n'; break;
            case 2: promo_char = 'b'; break;
            case 3: promo_char = 'r'; break;
            case 4: promo_char = 'q'; break;
        }
        printf("%c", promo_char);
    }
}

void print_move_indices(int from, int to) {
    printf("  [from=%d, to=%d]", from, to);
}

int main(void) {
    int fail = 0;
    for (int i = 0; i < NUM_TESTS; ++i) {
        Position pos;
        char err[256];
        if (position_from_fen(&pos, tests[i].fen, err, sizeof(err)) != POS_OK) {
            printf("Test %d: FEN error: %s\n", i+1, err);
            fail++; continue;
        }
        int from=-1, to=-1, promo=0;
        SearchContext ctx;
        search_context_init(&ctx, 6, 2000); // depth, ms
        search_root(&pos, 6, &from, &to, &promo, &ctx);

        int pass = from==tests[i].expected_from && to==tests[i].expected_to && promo==tests[i].expected_promo;

        printf("Test %2d %s: %s\n", i+1, tests[i].desc, pass ? "PASS" : "FAIL");
        printf("    Expected: ");
        print_move_uci(tests[i].expected_from, tests[i].expected_to, tests[i].expected_promo);
        print_move_indices(tests[i].expected_from, tests[i].expected_to);
        printf("\n    Got:      ");
        print_move_uci(from, to, promo);
        print_move_indices(from, to);
        printf("\n");

        if (!pass) fail++;
    }
    printf("=== Tactics tests: %d/%d passed ===\n", NUM_TESTS-fail, NUM_TESTS);
    return fail ? 1 : 0;
}
