#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "position.h"
#include "movegen.h"

static double now_seconds(void) {
    return (double)clock() / (double)CLOCKS_PER_SEC;
}

int main(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <FEN> <depth> [expected]\n", argv[0]);
        return 2;
    }

    const char *fen = argv[1];
    int depth = atoi(argv[2]);
    unsigned long long expected = 0;
    int have_expected = 0;
    if (argc >= 4) {
        expected = strtoull(argv[3], NULL, 10);
        have_expected = 1;
    }

    Position pos;
    char err[256];
    if (position_from_fen(&pos, fen, err, sizeof err) != POS_OK) {
        fprintf(stderr, "position_from_fen failed: %s\n", err);
        return 3;
    }

    double t0 = now_seconds();
    uint64_t nodes = perft(&pos, depth);
    double t1 = now_seconds();

    printf("FEN: %s\n", fen);
    printf("Depth: %d  Nodes: %llu  Time: %.3fs  nps: %.0f\n",
           depth, (unsigned long long)nodes, t1 - t0,
           (t1 - t0) > 0.0 ? (double)nodes / (t1 - t0) : (double)nodes);

    if (have_expected) {
        if (nodes == expected) {
            printf("Result: OK (matches expected %llu)\n", (unsigned long long)expected);
            return 0;
        } else {
            printf("Result: MISMATCH (expected %llu)\n", (unsigned long long)expected);
            return 4;
        }
    }

    return 0;
}
