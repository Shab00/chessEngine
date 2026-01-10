#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "position.h"
#include "movegen.h"

static void sq_to_coord(int sq, char *buf) {
    position_square_to_coords(sq, buf, 3);
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <FEN>\n", argv[0]);
        return 2;
    }
    const char *fen = argv[1];
    Position root;
    char err[256];
    if (position_from_fen(&root, fen, err, sizeof err) != POS_OK) {
        fprintf(stderr, "position_from_fen failed: %s\n", err);
        return 3;
    }

    int cap = 512;
    int *from = malloc(sizeof(int)*cap);
    int *to   = malloc(sizeof(int)*cap);
    int *prom = malloc(sizeof(int)*cap);
    if (!from || !to || !prom) { fprintf(stderr,"alloc failed\n"); return 4; }

    int n = generate_legal_moves(&root, from, to, prom, cap);
    printf("Root generated %d legal moves\n", n);

    for (int i = 0; i < n; ++i) {
        Position child;
        memcpy(&child, &root, sizeof(Position));
        MoveUndo undo_root;
        make_move(&child, from[i], to[i], prom[i], &undo_root);

        int m = generate_legal_moves(&child, from, to, prom, cap);
        for (int j = 0; j < m; ++j) {
            Position copy;
            memcpy(&copy, &child, sizeof(Position));
            MoveUndo undo;
            make_move(&copy, from[j], to[j], prom[j], &undo);
            unmake_move(&copy, &undo);
            if (memcmp(&copy, &child, sizeof(Position)) != 0) {
                char a1[4], b1[4], a2[4], b2[4];
                sq_to_coord(undo_root.from, a1);
                sq_to_coord(undo_root.to, b1);
                sq_to_coord(from[j], a2);
                sq_to_coord(to[j], b2);
                char froot[512], fchild[512], fafter[512];
                position_to_fen(&root, froot, sizeof froot);
                position_to_fen(&child, fchild, sizeof fchild);
                position_to_fen(&copy, fafter, sizeof fafter);
                printf("Corruption detected after applying root move %s%s and child move %s%s\n",
                       a1, b1, a2, b2);
                printf("root fen: %s\n", froot);
                printf("child fen before child move: %s\n", fchild);
                printf("child fen after unmake: %s\n", fafter);
                free(from); free(to); free(prom);
                return 0;
            }
        }
    }
    printf("No child-level corruption detected (all child replies restore child position)\n");
    free(from); free(to); free(prom);
    return 0;
}
