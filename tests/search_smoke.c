#include "position.h"
#include "search.h"
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    Position pos;
    char err[256];
    const char *start_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    if (position_from_fen(&pos, start_fen, err, sizeof err) != POS_OK) {
        fprintf(stderr, "FEN parse error: %s\n", err);
        return 2;
    }
    int from, to, promo;
    int ok = search_root(&pos, 1, &from, &to, &promo);
    if (!ok) {
        fprintf(stderr, "search returned no move\n");
        return 1;
    }
    return 0;
}
