#include "position.h"
#include "movegen.h"
#include "eval.h"
#include "search.h"
#include "search_context.h"
#include "search_order.h"
#include "tt.h"
#include "hash.h"
#include <stdio.h>

int main(void) {
    zobrist_init(0);
    tt_init(32);
    order_init(16);

    Position pos;
    char err[256];
    position_from_fen(&pos, "3k4/8/8/8/8/8/8/3QK3 w - - 0 1", err, sizeof(err));

    /* Evaluate the root position */
    int eval = evaluate(&pos);
    printf("Root eval (white perspective): %d\n", eval);
    int eval_stm = (pos.side_to_move == 0) ? eval : -eval;
    printf("Root eval (STM perspective): %d\n\n", eval_stm);

    /* Try each move manually at depth 1 */
    int froms[256], tos[256], promos[256];
    int n = generate_legal_moves(&pos, froms, tos, promos, 256);

    printf("Move-by-move eval after each move:\n");
    for (int i = 0; i < n; i++) {
        char f[4], t[4];
        position_square_to_coords(froms[i], f, sizeof f);
        position_square_to_coords(tos[i], t, sizeof t);

        MoveUndo undo;
        make_move(&pos, froms[i], tos[i], promos[i], &undo);

        int child_eval = evaluate(&pos);
        int child_stm = (pos.side_to_move == 0) ? child_eval : -child_eval;

        /* Check if opponent has any legal moves */
        int om[256], ot[256], op[256];
        int on = generate_legal_moves(&pos, om, ot, op, 256);

        /* Check if opponent king is in check */
        int in_check = position_king_in_check(&pos, pos.side_to_move);

        printf("  %s%s: eval=%d stm_eval=%d opp_moves=%d in_check=%d",
               f, t, child_eval, child_stm, on, in_check);
        if (on == 0 && in_check) printf(" ** MATE **");
        if (on == 0 && !in_check) printf(" ** STALEMATE **");
        printf("\n");

        unmake_move(&pos, &undo);
    }

    order_free();
    tt_free();
    return 0;
}
