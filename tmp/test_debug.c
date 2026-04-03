#include "position.h"
#include "movegen.h"
#include "eval.h"
#include "search.h"
#include "search_context.h"
#include "search_order.h"
#include "tt.h"
#include "hash.h"
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    zobrist_init(0);
    tt_init(32);
    order_init(16);

    Position pos;
    char err[256];
    position_from_fen(&pos, "3k4/8/8/8/8/8/8/3QK3 w - - 0 1", err, sizeof(err));

    int froms[256], tos[256], promos[256];
    int n = generate_legal_moves(&pos, froms, tos, promos, 256);

    SearchContext ctx;
    search_context_init(&ctx, 4, 10000);

    printf("Depth 1 search: trying each root move, calling search_ab(depth=0)\n");
    printf("alpha=-1000000000, beta=1000000000\n\n");

    int best_score = -1000000000;
    int best_i = -1;
    int alpha = -1000000000;
    int beta = 1000000000;

    for (int i = 0; i < n; i++) {
        char f[4], t[4];
        position_square_to_coords(froms[i], f, sizeof f);
        position_square_to_coords(tos[i], t, sizeof t);

        MoveUndo undo;
        make_move(&pos, froms[i], tos[i], promos[i], &undo);

        /* Check what evaluate returns in child */
        int raw_eval = evaluate(&pos);
        int stm_eval = (pos.side_to_move == 0) ? raw_eval : -raw_eval;

        /* Check opponent legal moves */
        int om[256], ot[256], op[256];
        int on = generate_legal_moves(&pos, om, ot, op, 256);

        unmake_move(&pos, &undo);

        /* Now do the real search call */
        make_move(&pos, froms[i], tos[i], promos[i], &undo);
        /* We need to replicate what search_root_once does:
           val = -search_ab(pos, depth-1, 1, -beta, -alpha, ctx)
           For depth=1: search_ab(depth=0) = quiescence */

        /* Manually compute: quiescence with wide window */
        /* stand_pat = evaluate_for_stm = stm_eval */
        /* If no captures, returns stand_pat */
        /* Caller negates: -stand_pat */

        int parent_sees = -stm_eval;

        printf("  %s%s: raw_eval=%d stm_eval=%d opp_moves=%d -stm=%d",
               f, t, raw_eval, stm_eval, on, parent_sees);
        if (on == 0) printf(" [TERMINAL]");

        if (parent_sees > best_score) {
            best_score = parent_sees;
            best_i = i;
            printf(" *BEST*");
        }
        printf("\n");

        unmake_move(&pos, &undo);
    }

    char bf[4], bt[4];
    position_square_to_coords(froms[best_i], bf, sizeof bf);
    position_square_to_coords(tos[best_i], bt, sizeof bt);
    printf("\nBest by -stm_eval: %s%s (score=%d)\n", bf, bt, best_score);

    printf("\n--- Now running actual search_root at depth 1 ---\n");
    int sf, st, sp;
    search_root(&pos, 1, &sf, &st, &sp, &ctx);
    char sfc[4], stc[4];
    position_square_to_coords(sf, sfc, sizeof sfc);
    position_square_to_coords(st, stc, sizeof stc);
    printf("search_root depth=1: %s%s\n", sfc, stc);

    order_free();
    tt_free();
    return 0;
}
