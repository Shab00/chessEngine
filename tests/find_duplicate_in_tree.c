#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "position.h"
#include "movegen.h"

#define MAX_PLY 64

static int cur_ply = 0;
static int duplicates_found = 0;

static int cmp_move(const void *a, const void *b) {
    const int *A = (const int*)a;
    const int *B = (const int*)b;
    if (A[0] != B[0]) return A[0] - B[0];
    if (A[1] != B[1]) return A[1] - B[1];
    return A[2] - B[2];
}

static void sq_to_coord(int sq, char *buf) { position_square_to_coords(sq, buf, 3); }

static void check_node(Position *pos, int depth) {
    if (depth == 0) return;
    int cap = 1024;
    int *from = malloc(sizeof(int)*cap);
    int *to   = malloc(sizeof(int)*cap);
    int *prom = malloc(sizeof(int)*cap);
    if (!from || !to || !prom) { free(from); free(to); free(prom); return; }
    int n = generate_legal_moves(pos, from, to, prom, cap);
    if (n > 1) {
        int (*moves)[3] = malloc(sizeof(int[3])*n);
        for (int i = 0; i < n; ++i) { moves[i][0]=from[i]; moves[i][1]=to[i]; moves[i][2]=prom[i]; }
        qsort(moves, n, sizeof(moves[0]), cmp_move);
        for (int i = 1; i < n; ++i) {
            if (moves[i][0]==moves[i-1][0] && moves[i][1]==moves[i-1][1] && moves[i][2]==moves[i-1][2]) {
                duplicates_found++;
                char a[4], b[4];
                sq_to_coord(moves[i][0], a); sq_to_coord(moves[i][1], b);
                printf("Duplicate legal move at ply %d: %s%s promo=%d\n", cur_ply, a, b, moves[i][2]);
                /* print current position FEN for diagnosis */
                char fen[512];
                position_to_fen(pos, fen, sizeof fen);
                printf(" Position FEN: %s\n", fen);
            }
        }
        free(moves);
    }
    for (int i = 0; i < n; ++i) {
        MoveUndo undo;
        make_move(pos, from[i], to[i], prom[i], &undo);
        cur_ply++;
        check_node(pos, depth - 1);
        cur_ply--;
        unmake_move(pos, &undo);
    }
    free(from); free(to); free(prom);
}

int main(int argc, char **argv) {
    if (argc < 3) { fprintf(stderr, "Usage: %s <FEN> <depth>\n", argv[0]); return 2; }
    const char *fen = argv[1];
    int depth = atoi(argv[2]);
    Position pos; char err[256];
    if (position_from_fen(&pos, fen, err, sizeof err) != POS_OK) { fprintf(stderr,"FEN parse failed: %s\n", err); return 3; }
    check_node(&pos, depth);
    if (duplicates_found == 0) printf("No duplicate legal moves found in tree (depth %d)\n", depth);
    else printf("Total duplicate occurrences found: %d\n", duplicates_found);
    return 0;
}
