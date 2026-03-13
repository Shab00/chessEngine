#include "search_order.h"
#include "movegen.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static int mvv_lva[7][7];

static uint32_t *history_table = NULL; /* size: 7 * 64 */

static uint32_t *killers = NULL;
static int killer_max_depth = 0;

static int piece_value_table[7] = {0, 100, 320, 330, 500, 900, 20000}; /* pawn..king */

static inline uint32_t pack_move_key(int from, int to, int promo)
{
    return (uint32_t)(from | (to << 6) | (promo << 12));
}

void order_init(int max_depth)
{
    if (killers) {
        order_free();
    }
    killer_max_depth = (max_depth > 0) ? max_depth : 64;
    killers = (uint32_t*)calloc((size_t)killer_max_depth * KILLER_SLOTS, sizeof(uint32_t));
    history_table = (uint32_t*)calloc(7 * 64, sizeof(uint32_t));

    for (int victim = 1; victim <= 6; ++victim) {
        for (int attacker = 1; attacker <= 6; ++attacker) {
            mvv_lva[victim][attacker] = piece_value_table[victim] * 100 - piece_value_table[attacker];
        }
    }
}

void order_free(void)
{
    if (killers) {
        free(killers);
        killers = NULL;
    }
    if (history_table) {
        free(history_table);
        history_table = NULL;
    }
    killer_max_depth = 0;
}

/* Return the captured piece for a move (handles en-passant).
   Returns PIECE_EMPTY if there is no capture. */
static inline int8_t get_captured_piece(const Position *pos, int from, int to)
{
    if (!pos) return PIECE_EMPTY;

    /* en-passant: captured pawn is not on 'to' square but behind it */
    if (pos->en_passant != POS_NO_SQUARE && to == pos->en_passant) {
        int8_t mover = pos->board[from];
        /* only pawns can capture en-passant */
        if (piece_abs(mover) != PIECE_PAWN) return PIECE_EMPTY;
        int dir = (mover > 0) ? -1 : 1; /* white pawn captures upward (rank+1), but we use index layout consistent with movegen */
        int cap_sq = to + dir * 8;
        if (cap_sq >= 0 && cap_sq < 64) return pos->board[cap_sq];
        return PIECE_EMPTY;
    }

    /* normal capture is just the piece on 'to' */
    return pos->board[to];
}

int score_move_from(const Position *pos, int from, int to, int promo, int ply)
{
    int score = 0;

    int8_t captured_piece = get_captured_piece(pos, from, to);
    if (captured_piece != PIECE_EMPTY) {
        int victim_abs = piece_abs(captured_piece);
        int attacker_abs = piece_abs(pos->board[from]);
        if (victim_abs >= 1 && victim_abs <= 6 && attacker_abs >= 1 && attacker_abs <= 6) {
            score = 1000000 + mvv_lva[victim_abs][attacker_abs]; /* captures dominate */
        } else {
            score = 900000;
        }
        return score;
    }

    if (promo != 0) {
        score = 800000 + (promo * 100);
        return score;
    }

    if (ply >= 0 && ply < killer_max_depth && killers) {
        uint32_t key = pack_move_key(from, to, promo);
        uint32_t k0 = killers[(size_t)ply * KILLER_SLOTS + 0];
        uint32_t k1 = killers[(size_t)ply * KILLER_SLOTS + 1];
        if (k0 == key && k0 != 0) return 500000;
        if (k1 == key && k1 != 0) return 499999;
    }

    int actor = piece_abs(pos->board[from]);
    if (actor >= 0 && actor < 7 && to >= 0 && to < 64 && history_table) {
        score = (int)history_table[actor * 64 + to];
    }

    return score;
}

void score_moves_from(const Position *pos,
                      const int *froms, const int *tos, const int *promos,
                      int n_moves, int ply, int *out_scores)
{
    for (int i = 0; i < n_moves; ++i) {
        int promo = promos ? promos[i] : 0;
        out_scores[i] = score_move_from(pos, froms[i], tos[i], promo, ply);
    }
}

void update_killers_from(int ply, int from, int to, int promo)
{
    if (ply < 0 || ply >= killer_max_depth || !killers) return;
    uint32_t key = pack_move_key(from, to, promo);
    uint32_t *base = &killers[(size_t)ply * KILLER_SLOTS];
    if (base[0] == key) return;
    if (base[1] == key) {
        base[1] = base[0];
        base[0] = key;
        return;
    }
    base[1] = base[0];
    base[0] = key;
}

void update_history_from(const Position *pos, int from, int to, int promo, int depth)
{
    (void)promo;
    if (!history_table || !pos) return;
    int actor = piece_abs(pos->board[from]);
    if (actor < 1 || actor > 6) return;
    if (to < 0 || to >= 64) return;
    uint32_t delta = (uint32_t)(depth * depth + 1);
    history_table[actor * 64 + to] += delta;
}
