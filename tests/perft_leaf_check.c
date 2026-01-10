#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "position.h"
#include "movegen.h"

int is_square_attacked(const Position *pos, int sq, int by);

#define MAX_PLY 64

static int path_from[MAX_PLY];
static int path_to[MAX_PLY];
static int path_prom[MAX_PLY];
static int cur_ply = 0;
static int printed = 0;
static uint64_t bad_count = 0;

static void print_path(int ply) {
    char a[8], b[8];
    for (int i = 0; i < ply; ++i) {
        position_square_to_coords(path_from[i], a, sizeof a);
        position_square_to_coords(path_to[i], b, sizeof b);
        if (path_prom[i] != 0) printf("%s%s=%d ", a, b, path_prom[i]);
        else printf("%s%s ", a, b);
    }
    printf("\n");
}

static int kings_count_and_sq(const Position *pos, int *w_sq, int *b_sq) {
    int w = 0, b = 0;
    *w_sq = *b_sq = POS_NO_SQUARE;
    for (int i = 0; i < 64; ++i) {
        int8_t v = pos->board[i];
        if (v == PIECE_KING) { w++; *w_sq = i; }
        if (v == -PIECE_KING) { b++; *b_sq = i; }
    }
    return (w<<8) | b;
}

static int kings_adjacent(int w_sq, int b_sq) {
    if (w_sq == POS_NO_SQUARE || b_sq == POS_NO_SQUARE) return 0;
    int wf = SQ_FILE(w_sq), wr = SQ_RANK(w_sq);
    int bf = SQ_FILE(b_sq), br = SQ_RANK(b_sq);
    return (abs(wf-bf) <= 1 && abs(wr-br) <= 1);
}

static int both_kings_in_check(const Position *pos) {
    int w_sq = POS_NO_SQUARE, b_sq = POS_NO_SQUARE;
    kings_count_and_sq(pos, &w_sq, &b_sq);
    if (w_sq == POS_NO_SQUARE || b_sq == POS_NO_SQUARE) return 0;
    int white_in_check = is_square_attacked(pos, w_sq, COLOR_BLACK);
    int black_in_check = is_square_attacked(pos, b_sq, COLOR_WHITE);
    return white_in_check && black_in_check;
}

static void check_leaf(const Position *pos) {
    int w_sq = POS_NO_SQUARE, b_sq = POS_NO_SQUARE;
    int kc = kings_count_and_sq(pos, &w_sq, &b_sq);
    int w = (kc >> 8) & 0xff;
    int b = kc & 0xff;
    int illegal = 0;
    if (w != 1 || b != 1) illegal = 1;
    if (kings_adjacent(w_sq, b_sq)) illegal = 1;
    if (both_kings_in_check(pos)) illegal = 1;
    if (illegal) {
        bad_count++;
        if (!printed) {
            printed = 1;
            printf("Found illegal leaf at path:\n");
            print_path(cur_ply);
            char before[512];
            position_to_fen((Position*)pos, before, sizeof before);
            printf("Leaf FEN: %s\n", before);
        }
    }
}

static uint64_t collect_perft(Position *pos, int depth) {
    if (depth == 0) {
        check_leaf(pos);
        return 1ULL;
    }
    int max_moves = 512;
    int *from = malloc(sizeof(int)*max_moves);
    int *to   = malloc(sizeof(int)*max_moves);
    int *prom = malloc(sizeof(int)*max_moves);
    if (!from || !to || !prom) { free(from); free(to); free(prom); return 0; }
    int n = generate_legal_moves(pos, from, to, prom, max_moves);
    uint64_t nodes = 0;
    for (int i = 0; i < n; ++i) {
        MoveUndo undo;
        make_move(pos, from[i], to[i], prom[i], &undo);
        path_from[cur_ply] = from[i];
        path_to[cur_ply] = to[i];
        path_prom[cur_ply] = prom[i];
        cur_ply++;
        nodes += collect_perft(pos, depth - 1);
        cur_ply--;
        unmake_move(pos, &undo);
    }
    free(from); free(to); free(prom);
    return nodes;
}

int main(int argc, char **argv) {
    if (argc < 3) { fprintf(stderr, "Usage: %s <FEN> <depth>\n", argv[0]); return 2; }
    const char *fen = argv[1];
    int depth = atoi(argv[2]);
    Position pos; char err[256];
    if (position_from_fen(&pos, fen, err, sizeof err) != POS_OK) { fprintf(stderr, "FEN parse failed: %s\n", err); return 3; }
    uint64_t nodes = collect_perft(&pos, depth);
    printf("Collected leaf nodes: %llu\n", (unsigned long long)nodes);
    printf("Illegal leaf count: %u\n", (unsigned)bad_count);
    return 0;
}
