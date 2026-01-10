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

static void emit_leaf(Position *pos) {
    char fen[512];
    position_to_fen(pos, fen, sizeof fen);
    printf("%s\n", fen);
}

static void collect_leaves(Position *pos, int depth) {
    if (depth == 0) {
        emit_leaf(pos);
        return;
    }
    int cap = 512;
    int *from = malloc(sizeof(int)*cap);
    int *to   = malloc(sizeof(int)*cap);
    int *prom = malloc(sizeof(int)*cap);
    if (!from || !to || !prom) { free(from); free(to); free(prom); return; }
    int n = generate_legal_moves(pos, from, to, prom, cap);
    for (int i = 0; i < n; ++i) {
        MoveUndo undo;
        make_move(pos, from[i], to[i], prom[i], &undo);
        path_from[cur_ply] = from[i];
        path_to[cur_ply] = to[i];
        path_prom[cur_ply] = prom[i];
        cur_ply++;
        collect_leaves(pos, depth - 1);
        cur_ply--;
        unmake_move(pos, &undo);
    }
    free(from); free(to); free(prom);
}

int main(int argc, char **argv) {
    if (argc < 3) { fprintf(stderr, "Usage: %s <FEN> <depth>\n", argv[0]); return 2; }
    const char *fen = argv[1]; int depth = atoi(argv[2]);
    Position pos; char err[256];
    if (position_from_fen(&pos, fen, err, sizeof err) != POS_OK) { fprintf(stderr, "FEN parse failed: %s\n", err); return 3; }
    collect_leaves(&pos, depth);
    return 0;
}
