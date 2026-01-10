#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "position.h"
#include "movegen.h"

static void sq_to_coord(int sq, char *buf, size_t n) { position_square_to_coords(sq, buf, n); }

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s \"<FEN>\"\n", argv[0]);
        return 2;
    }
    const char *fen = argv[1];
    Position pos; char err[256];
    if (position_from_fen(&pos, fen, err, sizeof err) != POS_OK) {
        fprintf(stderr, "FEN parse failed: %s\n", err);
        return 3;
    }
    int cap = 1024;
    int *from = malloc(sizeof(int)*cap);
    int *to   = malloc(sizeof(int)*cap);
    int *prom = malloc(sizeof(int)*cap);
    if (!from || !to || !prom) { free(from); free(to); free(prom); return 4; }
    int n = generate_legal_moves(&pos, from, to, prom, cap);
    printf("Generated %d moves:\n", n);
    for (int i = 0; i < n; ++i) {
        char a[8], b[8];
        sq_to_coord(from[i], a, sizeof a);
        sq_to_coord(to[i], b, sizeof b);
        MoveUndo undo;
        make_move(&pos, from[i], to[i], prom[i], &undo);
        char fen2[512];
        position_to_fen(&pos, fen2, sizeof fen2);
        unmake_move(&pos, &undo);
        if (prom[i] != 0) printf("%3d: %s%s=%d -> %s\n", i+1, a, b, prom[i], fen2);
        else           printf("%3d: %s%s -> %s\n", i+1, a, b, fen2);
    }
    free(from); free(to); free(prom);
    return 0;
}
