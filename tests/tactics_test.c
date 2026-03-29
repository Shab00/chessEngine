#include <stdio.h>
#include <string.h>
#include "movegen.h"
#include "position.h"
#include "print.h"

typedef struct {
    const char *fen;
    int expected_from, expected_to, expected_promo;
    const char *desc;
} TacticTest;
TacticTest tests[] = {
    // Test 1: Qd1-d8#
    {
        "3k4/8/8/8/8/8/8/3QK3 w - - 0 1",
        3,  // d1
        59, // d8
        0,
        "Simple mate in 1: Qd1-d8#"
    },
    // Test 2: Qg7-h7#
    {
        "7k/5KQ1/8/8/8/8/8/8 w - - 0 1",
        54, // g7
        55, // h7
        0,
        "Simple mate in 1: Qg7-h7#"
    },
    // Test 3: White plays c4-c5
    {
        "8/5k1p/1p1pRp2/p2P4/P1P3Pp/1P4bP/6K1/8 w - - 0 49",
        26, // c4
        34, // c5
        0,
        "White plays c4-c5"
    },

    // 01: bm Nf6+
    {"1rbq1rk1/p1b1nppp/1p2p3/8/1B1pN3/P2B4/1P3PPP/2RQ1R1K w - - 0 1", 36, 45, 0, "bm Nf6+"}, // N e4-f6

    // 02: bm Nxd4
    {"3r2k1/p2r1p1p/1p2p1p1/q4n2/3P4/PQ5P/1P1RNPP1/3R2K1 b - - 0 1", 21, 27, 0, "bm Nxd4"}, // N f5-d4

    // 03: am Rd1
    {"3r2k1/1p3ppp/2pq4/p1n5/P6P/1P6/1PB2QP1/1K2R3 w - - 0 1", 20, 3, 0, "am Rd1"},

    // 04: bm Rxb2
    {"r1b1r1k1/1ppn1p1p/3pnqp1/8/p1P1P3/5P2/PbNQNBPP/1R2RB1K w - - 0 1", 9, 49, 0, "bm Rxb2"}, // R b1xb2

    // 05: bm Qxc1
    {"2r4k/pB4bp/1p4p1/6q1/1P1n4/2N5/P4PPP/2R1Q1K1 b - - 0 1", 31, 58, 0, "bm Qxc1"}, // Q g5xc1

    // 06: am Rxa2
    {"r5k1/3n1ppp/1p6/3p1p2/3P1B2/r3P2P/PR3PP1/2R3K1 b - - 0 1", 40, 48, 0, "am Rxa2"}, // R a3xa2

    // 07: bm Bxe4
    {"2r2rk1/1bqnbpp1/1p1ppn1p/pP6/N1P1P3/P2B1N1P/1B2QPP1/R2R2K1 b - - 0 1", 25, 28, 0, "bm Bxe4"}, // B d7xe4

    // 08: bm h6
    {"5r1k/6pp/1n2Q3/4p3/8/7P/PP4PK/R1B1q3 b - - 0 1", 23, 31, 0, "bm h6"}, // h7-h6

    // 09: bm Be2
    {"r3k2r/pbn2ppp/8/1P1pP3/P1qP4/5B2/3Q1PPP/R3K2R w KQkq - 0 1", 20, 12, 0, "bm Be2"}, // B e3-e2

    // 10: bm Nxc3
    {"3r2k1/ppq2pp1/4p2p/3n3P/3N2P1/2P5/PP2QP2/K2R4 b - - 0 1", 27, 18, 0, "bm Nxc3"}, // N d5xc3

    // 11: bm Nf5
    {"q3rn1k/2QR4/pp2pp2/8/P1P5/1P4N1/6n1/6K1 w - - 0 1", 46, 37, 0, "bm Nf5"}, // N g3-f5

    // 12: bm Rd6
    {"6k1/p3q2p/1nr3pB/8/3Q1P2/6P1/PP5P/3R2K1 b - - 0 1", 17, 43, 0, "bm Rd6"}, // R d8-d6

    // 13: bm Nxd5
    {"1r4k1/7p/5np1/3p3n/8/2NB4/7P/3N1RK1 w - - 0 1", 50, 35, 0, "bm Nxd5"}, // N d3xd5

    // 14: bm Rxb2
    {"1r2r1k1/p4p1p/6pB/q7/8/3Q2P1/PbP2PKP/1R3R2 w - - 0 1", 57, 49, 0, "bm Rxb2"}, // R b1xb2

    // 15: bm Bxf5
    {"r2q1r1k/pb3p1p/2n1p2Q/5p2/8/3B2N1/PP3PPP/R3R1K1 w - - 0 1", 35, 45, 0, "bm Bxf5"}, // B d3xf5

    // 16: bm b4
    {"8/4p3/p2p4/2pP4/2P1P3/1P4k1/1P1K4/8 w - - 0 1", 17, 25, 0, "bm b4"}, // b3-b4

    // 17: bm e5
    {"1r1q1rk1/p1p2pbp/2pp1np1/6B1/4P3/2NQ4/PPP2PPP/3R1RK1 w - - 0 1", 36, 28, 0, "bm e5"}, // e4-e5

    // 18: bm Qc8
    {"q4rk1/1n1Qbppp/2p5/1p2p3/1P2P3/2P4P/6P1/2B1NRK1 b - - 0 1", 17, 58, 0, "bm Qc8"}, // Q d7-c8

    // 19: bm Qxd7
    {"r2q1r1k/1b1nN2p/pp3pp1/8/Q7/PP5P/1BP2RPN/7K w - - 0 1", 24, 27, 0, "bm Qxd7"}, // Q a4xd7

    // 20: bm Bg4
    {"8/5p2/pk2p3/4P2p/2b1pP1P/P3P2B/8/7K w - - 0 1", 43, 30, 0, "bm Bg4"}, // B h3-g4

    // 21: bm Kxb5
    {"8/2k5/4p3/1nb2p2/2K5/8/6B1/8 w - - 0 1", 34, 41, 0, "bm Kxb5"}, // K c4xb5

    // 22: bm Ba7
    {"1B1b4/7K/1p6/1k6/8/8/8/8 w - - 0 1", 56, 48, 0, "bm Ba7"}, // B b8-a7

    // 23: bm Ba6
    {"rn1q1rk1/1b2bppp/1pn1p3/p2pP3/3P4/P2BBN1P/1P1N1PP1/R2Q1RK1 b - - 0 1", 9, 40, 0, "bm Ba6"}, // B b7-a6

    // 24: bm Bxc6
    {"8/p1ppk1p1/2n2p2/8/4B3/2P1KPP1/1P5P/8 w - - 0 1", 36, 18, 0, "bm Bxc6"}, // B e4xc6

    // 25: bm Bxd7
    {"8/3nk3/3pp3/1B6/8/3PPP2/4K3/8 w - - 0 1", 41, 27, 0, "bm Bxd7"}, // B b5xd7
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

int main(void) {
    int pass_count = 0;
    for (int i = 0; i < NUM_TESTS; ++i) {
        Position pos;
        char err[256];
        if (position_from_fen(&pos, tests[i].fen, err, sizeof(err)) != POS_OK) {
            printf("Test %d: FEN error: %s\n", i+1, err);
            continue;
        }

        // Generate all moves
        int froms[512], tos[512], promos[512];
        int n_moves = generate_pseudo_moves(&pos, froms, tos, promos, 512);

        // Check for expected queen move (and also show the first queen move from expected square)
        int found = 0, first_idx = -1;
        for (int m = 0; m < n_moves; ++m) {
            if (froms[m] == tests[i].expected_from) {
                if (first_idx == -1) first_idx = m;
                if (froms[m] == tests[i].expected_from &&
                    tos[m] == tests[i].expected_to &&
                    promos[m] == tests[i].expected_promo) {
                    found = 1;
                }
            }
        }

        printf("Test %2d: %s\n", i+1, tests[i].desc);
        printf("  Expected: ");
        print_move_uci(tests[i].expected_from, tests[i].expected_to, tests[i].expected_promo);
        printf("  [from=%d, to=%d]\n", tests[i].expected_from, tests[i].expected_to);

        if (first_idx != -1) {
            printf("  First generated move from that square: ");
            print_move_uci(froms[first_idx], tos[first_idx], promos[first_idx]);
            printf("  [from=%d, to=%d]\n", froms[first_idx], tos[first_idx]);
        } else {
            printf("  No generated moves from that square.\n");
        }

        printf("  Result: %s\n\n", found ? "PASS" : "FAIL");
        if (found) pass_count++;
    }
    printf("=== Tactics tests: %d/%d passed ===\n", pass_count, NUM_TESTS);
    return (pass_count == NUM_TESTS ? 0 : 1);
}
