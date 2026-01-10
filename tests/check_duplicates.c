#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "position.h"
#include "movegen.h"

static int cmp_move(const void *a, const void *b) {
    const int *A = (const int*)a;
    const int *B = (const int*)b;
    if (A[0] != B[0]) return A[0] - B[0];
    if (A[1] != B[1]) return A[1] - B[1];
    return A[2] - B[2];
}

static void print_sq(int sq) {
    char s[8];
    position_square_to_coords(sq, s, sizeof s);
    printf("%s", s);
}

int main(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "Usage: %s <FEN>\n", argv[0]); return 2; }
    const char *fen = argv[1];
    Position pos;
    char err[256];
    if (position_from_fen(&pos, fen, err, sizeof err) != POS_OK) {
        fprintf(stderr, "FEN parse failed: %s\n", err);
        return 3;
    }

    int cap = 2048;
    int *from = malloc(sizeof(int)*cap);
    int *to   = malloc(sizeof(int)*cap);
    int *prom = malloc(sizeof(int)*cap);
    if (!from||!to||!prom) { fprintf(stderr,"alloc failed\n"); return 4; }

    int n = generate_legal_moves(&pos, from, to, prom, cap);
    if (n <= 0) { printf("No legal moves\n"); return 0; }

    int (*moves)[3] = malloc(sizeof(int[3])*n);
    for (int i=0;i<n;++i) { moves[i][0]=from[i]; moves[i][1]=to[i]; moves[i][2]=prom[i]; }
    qsort(moves, n, sizeof(moves[0]), cmp_move);

    int dup_found = 0;
    for (int i=1;i<n;++i) {
        if (moves[i][0]==moves[i-1][0] && moves[i][1]==moves[i-1][1] && moves[i][2]==moves[i-1][2]) {
            if (!dup_found) printf("Duplicate legal moves in FEN: %s\n", fen);
            dup_found = 1;
            print_sq(moves[i][0]); print_sq(moves[i][1]);
            if (moves[i][2]) printf(" promo=%d", moves[i][2]);
            printf("  (duplicate)\n");
        }
    }
    if (!dup_found) printf("No duplicate legal moves found\n");
    free(from); free(to); free(prom); free(moves);
    return 0;
}
