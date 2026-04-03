#include "position.h"
#include "movegen.h"
#include "search.h"
#include "search_context.h"
#include "tt.h"
#include "hash.h"
#include <stdio.h>

int main(void) {
    zobrist_init(0);
    tt_init(32);

    Position pos;
    char err[256];
    position_from_fen(&pos, "3k4/8/8/8/8/8/8/3QK3 w - - 0 1", err, sizeof(err));

    printf("Hash: %llu\n", (unsigned long long)pos.hash);
    printf("Board[3] (d1): %d  Board[4] (e1): %d  Board[59] (d8): %d\n",
           pos.board[3], pos.board[4], pos.board[59]);
    printf("d8 attacked by white: %d\n", position_is_square_attacked(&pos, 59, 0));

    int froms[256], tos[256], promos[256];
    int n = generate_legal_moves(&pos, froms, tos, promos, 256);
    printf("\nLegal moves (%d):\n", n);
    for (int i = 0; i < n; i++) {
        char f[4], t[4];
        position_square_to_coords(froms[i], f, sizeof f);
        position_square_to_coords(tos[i], t, sizeof t);
        printf("  %s%s", f, t);
        MoveUndo undo;
        make_move(&pos, froms[i], tos[i], promos[i], &undo);
        int om[256], ot[256], op[256];
        int on = generate_legal_moves(&pos, om, ot, op, 256);
        if (on == 0) printf(" [CHECKMATE!]");
        unmake_move(&pos, &undo);
    }
    printf("\n\n");

    SearchContext ctx;
    search_context_init(&ctx, 2, 10000);
    int from = -1, to = -1, promo = 0;
    search_root(&pos, 2, &from, &to, &promo, &ctx);
    char fs[4], ts[4];
    position_square_to_coords(from, fs, sizeof fs);
    position_square_to_coords(to, ts, sizeof ts);
    printf("Depth 2 best move: %s%s (from=%d to=%d)\n", fs, ts, from, to);

    tt_free();
    return 0;
}
