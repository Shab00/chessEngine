#include "search.h"
#include "tt.h"
#include "position.h"
#include "search_context.h"
#include <stdlib.h>
#include <string.h>

static int preferred_from = -1;
static int preferred_to = -1;
static int preferred_promo = 0;

static const int ord_piece_value[7] = { 0, 100, 320, 330, 500, 900, 20000 };

static int score_move_mvv_lva(const Position *pos, int from, int to, int promo)
{
    if (from < 0 || from >= 64 || to < 0 || to >= 64) return 0;
    int8_t cap = pos->board[to];
    int cap_abs = cap == 0 ? 0 : (cap > 0 ? cap : -cap);
    int8_t att = pos->board[from];
    int att_abs = att == 0 ? 0 : (att > 0 ? att : -att);
    int cap_val = (cap_abs >= 0 && cap_abs <= 6) ? ord_piece_value[cap_abs] : 0;
    int att_val = (att_abs >= 0 && att_abs <= 6) ? ord_piece_value[att_abs] : 0;
    int score = cap_val * 100 - att_val;
    if (promo != 0) score += 10000 + (ord_piece_value[promo] * 10);
    if (from == preferred_from && to == preferred_to && promo == preferred_promo) score += 2000000;
    return score;
}

void reorder_moves(int *froms, int *tos, int *promos, int n, const Position *pos)
{
    for (int i = 1; i < n; ++i) {
        int fi = froms[i], ti = tos[i], pi = promos[i];
        int si = score_move_mvv_lva(pos, fi, ti, pi);
        int j = i - 1;
        while (j >= 0) {
            int sj = score_move_mvv_lva(pos, froms[j], tos[j], promos[j]);
            if (sj >= si) break;
            froms[j + 1] = froms[j];
            tos[j + 1] = tos[j];
            promos[j + 1] = promos[j];
            --j;
        }
        froms[j + 1] = fi;
        tos[j + 1] = ti;
        promos[j + 1] = pi;
    }
}

int search_iterative_deepening(Position *pos, int max_depth, int *out_from, int *out_to, int *out_promotion)
{
    int bf = -1, bt = -1, bp = 0;
    for (int depth = 1; depth <= max_depth; ++depth) {
        preferred_from = bf; preferred_to = bt; preferred_promo = bp;

        SearchContext ctx;
        search_context_init(&ctx, depth, 0);

        int ok = search_root(pos, depth, &bf, &bt, &bp, &ctx);
        if (!ok) break;
    }

    if (bf >= 0) {
        if (out_from) *out_from = bf;
        if (out_to) *out_to = bt;
        if (out_promotion) *out_promotion = bp;
        return 1;
    }
    return 0;
}
