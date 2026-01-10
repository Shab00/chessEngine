#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "position.h"
#include "movegen.h"

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <FEN>\n", argv[0]);
        return 2;
    }
    const char *fen = argv[1];
    Position pos;
    char err[256];
    if (position_from_fen(&pos, fen, err, sizeof err) != POS_OK) {
        fprintf(stderr, "position_from_fen failed: %s\n", err);
        return 3;
    }

    int max = 512;
    int *from = malloc(sizeof(int)*max);
    int *to   = malloc(sizeof(int)*max);
    int *prom = malloc(sizeof(int)*max);
    if (!from || !to || !prom) { fprintf(stderr, "alloc failed\n"); return 4; }

    int n = generate_legal_moves(&pos, from, to, prom, max);
    printf("Generated %d legal moves\n", n);

    for (int i = 0; i < n; ++i) {
        Position copy;
        memcpy(&copy, &pos, sizeof(Position));
        MoveUndo undo;
        make_move(&copy, from[i], to[i], prom[i], &undo);
        unmake_move(&copy, &undo);
        if (memcmp(&copy, &pos, sizeof(Position)) != 0) {
            char a[4], b[4], fbuf[256], fbuf2[256];
            position_square_to_coords(from[i], a, sizeof a);
            position_square_to_coords(to[i], b, sizeof b);
            position_to_fen(&pos, fbuf, sizeof fbuf);
            position_to_fen(&copy, fbuf2, sizeof fbuf2);
            printf("MOVE %d %s%s prom=%d did NOT restore position\n", i+1, a, b, prom[i]);
            printf(" original fen: %s\n", fbuf);
            printf(" after unmake: %s\n", fbuf2);
            free(from); free(to); free(prom);
            return 0;
        }
    }
    printf("All moves restored positions correctly\n");
    free(from); free(to); free(prom);
    return 0;
}
