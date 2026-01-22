#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <ctype.h>

#include "position.h"
#include "movegen.h"

typedef unsigned long long u64;

static int parse_fen_wrapper(Position *pos, const char *fen) {
    char errbuf[256] = {0};
    pos_error_t err = position_from_fen(pos, fen, errbuf, sizeof(errbuf));
    if (err != POS_OK) {
        if (errbuf[0]) fprintf(stderr, "position_from_fen error: %s\n", errbuf);
        else fprintf(stderr, "position_from_fen returned error code %d\n", (int)err);
        return 0;
    }
    return 1;
}

static int parse_test_line(const char *line_in, char **fen_out, int *depth_out, unsigned long long *expected_out) {
    if (!line_in || !fen_out || !depth_out || !expected_out) return 0;
    char buf[1024];
    size_t len = strlen(line_in);
    if (len >= sizeof(buf)) return 0;
    memcpy(buf, line_in, len + 1);

    while (len > 0 && (buf[len-1] == '\n' || buf[len-1] == '\r' || isspace((unsigned char)buf[len-1]))) {
        buf[--len] = '\0';
    }
    if (len == 0) return 0;

    char *p = buf + len - 1;
    while (p >= buf && isspace((unsigned char)*p)) --p;
    if (p < buf) return 0;
    char *end_expected = p;
    while (p >= buf && !isspace((unsigned char)*p)) --p;
    char *start_expected = p + 1;
    unsigned long long expected = strtoull(start_expected, NULL, 10);

    p = start_expected - 1;
    while (p >= buf && isspace((unsigned char)*p)) --p;
    if (p < buf) return 0;
    char *end_depth = p;
    while (p >= buf && !isspace((unsigned char)*p)) --p;
    char *start_depth = p + 1;
    int depth = (int)strtol(start_depth, NULL, 10);

    size_t fen_len = (size_t)(p - buf + 1);
    while (fen_len > 0 && isspace((unsigned char)buf[fen_len - 1])) fen_len--;
    if (fen_len == 0) return 0;

    char *fen = (char *)malloc(fen_len + 1);
    if (!fen) return 0;
    memcpy(fen, buf, fen_len);
    fen[fen_len] = '\0';

    *fen_out = fen;
    *depth_out = depth;
    *expected_out = expected;
    return 1;
}

u64 perft_rec(Position *pos, int depth) {
    if (depth == 0) return 1ULL;

    int moves_from[256], moves_to[256], promotions[256];
    int n = generate_legal_moves(pos, moves_from, moves_to, promotions, 256);
    u64 nodes = 0;

    for (int i = 0; i < n; ++i) {
        MoveUndo undo = {0};
        int from = moves_from[i];
        int to = moves_to[i];
        int promo = promotions[i];
        make_move(pos, from, to, promo, &undo);
        nodes += perft_rec(pos, depth - 1);
        unmake_move(pos, &undo);
    }
    return nodes;
}

void perft_divide(Position *pos, int depth) {
    int moves_from[256], moves_to[256], promotions[256];
    int n = generate_legal_moves(pos, moves_from, moves_to, promotions, 256);

    for (int i = 0; i < n; ++i) {
        MoveUndo undo = {0};
        int from = moves_from[i];
        int to = moves_to[i];
        int promo = promotions[i];
        make_move(pos, from, to, promo, &undo);
        u64 cnt = perft_rec(pos, depth - 1);
        unmake_move(pos, &undo);
        printf("%d->%d%s: %llu\n", from, to, promo ? " (promo)" : "", cnt);
    }
}

int run_test(const char *fen, int depth, u64 expected) {
    Position pos;
    if (!parse_fen_wrapper(&pos, fen)) {
        fprintf(stderr, "parse_fen failed for: %s\n", fen);
        return 1;
    }
    u64 got = perft_rec(&pos, depth);
    if (got != expected) {
        printf("FAIL FEN=\"%s\" depth=%d expected=%llu got=%llu\n", fen, depth, expected, got);
        printf("Perft divide for failing position (depth %d):\n", depth);
        perft_divide(&pos, depth);
        return 1;
    }
    printf("OK FEN depth=%d nodes=%llu\n", depth, got);
    return 0;
}

int main(int argc, char **argv) {
    if (argc >= 4 && strcmp(argv[1], "divide") == 0) {
        Position pos;
        if (!parse_fen_wrapper(&pos, argv[2])) { fprintf(stderr, "parse_fen failed\n"); return 1; }
        int depth = atoi(argv[3]);
        perft_divide(&pos, depth);
        return 0;
    }

    if (argc == 2 && strcmp(argv[1], "runfile") == 0) {
        FILE *f = fopen("tests/perft_tests.txt", "r");
        if (!f) { perror("open tests/perft_tests.txt"); return 1; }
        char line[1024];
        int failures = 0;
        while (fgets(line, sizeof(line), f)) {
            if (line[0] == '#' || line[0] == '\n') continue;
            char *fen = NULL;
            int depth = 0;
            unsigned long long expected = 0;
            if (!parse_test_line(line, &fen, &depth, &expected)) {
                fprintf(stderr, "Skipping malformed line: %s", line);
                continue;
            }
            if (run_test(fen, depth, expected) != 0) failures++;
            free(fen);
        }
        fclose(f);
        return failures == 0 ? 0 : 2;
    }

    if (argc == 3) {
        Position pos;
        if (!parse_fen_wrapper(&pos, argv[1])) { fprintf(stderr, "parse_fen failed for %s\n", argv[1]); return 1; }
        int depth = atoi(argv[2]);
        unsigned long long nodes = perft_rec(&pos, depth);
        printf("%llu\n", nodes);
        return 0;
    }

    fprintf(stderr, "Usage:\n  %s \"FEN\" depth\n  %s divide \"FEN\" depth\n  %s runfile\n", argv[0], argv[0], argv[0]);
    return 1;
}
