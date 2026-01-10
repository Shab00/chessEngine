#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "position.h"
#include "movegen.h"

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <FEN>\n", argv[0]);
        return 2;
    }
    const char *fen = argv[1];
    Position pos;
    char err[256];
    if (position_from_fen(&pos, fen, err, sizeof err) != POS_OK) {
        fprintf(stderr, "FEN parse failed: %s\n", err);
        return 3;
    }

    Position before;
    memcpy(&before, &pos, sizeof(Position));

    int cap = 1024;
    int *from = malloc(sizeof(int)*cap);
    int *to   = malloc(sizeof(int)*cap);
    int *prom = malloc(sizeof(int)*cap);

    int n1 = generate_legal_moves(&pos, from, to, prom, cap);

    if (memcmp(&before, &pos, sizeof(Position)) != 0) {
        char bfen[512], afen[512];
        position_to_fen(&before, bfen, sizeof bfen);
        position_to_fen(&pos, afen, sizeof afen);
        printf("Mutation detected AFTER first generate_legal_moves()\n");
        printf("Before: %s\n", bfen);
        printf("After:  %s\n", afen);
    } else {
        printf("No mutation after first generate_legal_moves() (n=%d)\n", n1);
    }

    /* Call again to test idempotence / repeated calls */
    memcpy(&before, &pos, sizeof(Position));
    int n2 = generate_legal_moves(&pos, from, to, prom, cap);
    if (memcmp(&before, &pos, sizeof(Position)) != 0) {
        char bfen[512], afen[512];
        position_to_fen(&before, bfen, sizeof bfen);
        position_to_fen(&pos, afen, sizeof afen);
        printf("Mutation detected AFTER second generate_legal_moves()\n");
        printf("Before: %s\n", bfen);
        printf("After:  %s\n", afen);
    } else {
        printf("No mutation after second generate_legal_moves() (n=%d)\n", n2);
    }

    free(from); free(to); free(prom);
    return 0;
}
