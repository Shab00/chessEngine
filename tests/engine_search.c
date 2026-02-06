#include "position.h"
#include "search.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "usage: %s <FEN> <depth>\n", argv[0]);
        return 2;
    }
    const char *fen = argv[1];
    int depth = atoi(argv[2]);
    Position pos;
    char err[256];
    if (position_from_fen(&pos, fen, err, sizeof err) != POS_OK) {
        fprintf(stderr, "bad fen: %s\n", err);
        return 2;
    }
    int from, to, promo;
    int ok = search_root(&pos, depth, &from, &to, &promo);
    if (!ok) {
        fprintf(stdout, "no legal move found\n");
        return 0;
    }
    char from_s[4], to_s[4];
    position_square_to_coords(from, from_s, sizeof from_s);
    position_square_to_coords(to, to_s, sizeof to_s);
    if (promo != 0) {
        printf("best: %s->%s promote=%d\n", from_s, to_s, promo);
    } else {
        printf("best: %s->%s\n", from_s, to_s);
    }
    return 0;
}
