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

    /* Make d1d8 and check what quiescence returns */
    MoveUndo undo;
    make_move(&pos, 3, 59, 0, &undo);  /* d1d8 */

    printf("After d1d8:\n");
    printf("  side_to_move: %d (0=white, 1=black)\n", pos.side_to_move);

    int eval_w = evaluate(&pos);
    printf("  evaluate(): %d (white perspective)\n", eval_w);

    int eval_stm = (pos.side_to_move == 0) ? eval_w : -eval_w;
    printf("  evaluate_for_stm(): %d\n", eval_stm);

    int n_legal = 0;
    {
        int f[256], t[256], p[256];
        n_legal = generate_legal_moves(&pos, f, t, p, 256);
    }
    printf("  legal moves: %d\n", n_legal);
    printf("  king in check: %d\n", position_king_in_check(&pos, pos.side_to_move));

    /* What does search_ab see at depth 0? (quiescence) */
    /* We can't call search_ab directly, but we can reason:
       stand_pat = evaluate_for_stm = %d
       No captures available (queen just took nothing on d8, board is K vs Q)
       So quiescence returns stand_pat = %d
       Negated by caller: -%d
    */
    printf("\n  Quiescence would return stand_pat = %d\n", eval_stm);
    printf("  Caller sees: -%d = %d\n", eval_stm, -eval_stm);
    printf("  This should be a MATE score (~100000) but is only %d\n", -eval_stm);
    printf("\n  ==> QUIESCENCE DOES NOT DETECT MATE. It returns eval, not mate score.\n");

    unmake_move(&pos, &undo);

    order_free();
    tt_free();
    return 0;
}
