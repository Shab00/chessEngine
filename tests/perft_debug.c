#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "position.h"
#include "movegen.h"

#define MAX_PLY 64

static int path_from[MAX_PLY];
static int path_to[MAX_PLY];
static int path_prom[MAX_PLY];
static int cur_ply = 0;

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

static uint64_t debug_perft(Position *pos, int depth) {
    if (depth == 0) return 1ULL;
    int max_moves = 512;
    int *from = malloc(sizeof(int)*max_moves);
    int *to   = malloc(sizeof(int)*max_moves);
    int *prom = malloc(sizeof(int)*max_moves);
    if (!from || !to || !prom) { free(from); free(to); free(prom); return 0; }
    int n = generate_legal_moves(pos, from, to, prom, max_moves);
    uint64_t nodes = 0;
    for (int i = 0; i < n; ++i) {
        Position snapshot;
        memcpy(&snapshot, pos, sizeof(Position));
        MoveUndo undo;
        make_move(pos, from[i], to[i], prom[i], &undo);
        path_from[cur_ply] = from[i];
        path_to[cur_ply] = to[i];
        path_prom[cur_ply] = prom[i];
        cur_ply++;
        uint64_t sub = debug_perft(pos, depth - 1);
        cur_ply--;
        unmake_move(pos, &undo);
        if (memcmp(&snapshot, pos, sizeof(Position)) != 0) {
            printf("Position mismatch after unmake at ply %d\n", cur_ply);
            printf("Move path: ");
            print_path(cur_ply+1);
            char f1[512], f2[512];
            position_to_fen(&snapshot, f1, sizeof f1);
            position_to_fen(pos, f2, sizeof f2);
            printf("Before make (saved): %s\n", f1);
            printf("After unmake:       %s\n", f2);
            free(from); free(to); free(prom);
            exit(1);
        }
        nodes += sub;
    }
    free(from); free(to); free(prom);
    return nodes;
}

int main(int argc, char **argv) {
    if (argc < 3) { fprintf(stderr, "Usage: %s <FEN> <depth>\n", argv[0]); return 2; }
    const char *fen = argv[1]; int depth = atoi(argv[2]);
    Position pos; char err[256];
    if (position_from_fen(&pos, fen, err, sizeof err) != POS_OK) { fprintf(stderr, "FEN parse failed: %s\n", err); return 3; }
    uint64_t nodes = debug_perft(&pos, depth);
    printf("debug perft nodes: %llu\n", (unsigned long long)nodes);
    return 0;
}
