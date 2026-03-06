#include "hash.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static uint64_t zob_piece[64][12];
static uint64_t zob_side;
static uint64_t zob_castle[16];
static uint64_t zob_ep[8];

static uint64_t splitmix64(uint64_t *state) {
    uint64_t z = (*state += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

void zobrist_init(uint64_t seed)
{
    if (seed == 0) seed = 0xC0FFEE123456789ULL;
    uint64_t st = seed;

    for (int sq = 0; sq < 64; ++sq) {
        for (int p = 0; p < 12; ++p) {
            zob_piece[sq][p] = splitmix64(&st);
        }
    }
    zob_side = splitmix64(&st);
    for (int i = 0; i < 16; ++i) zob_castle[i] = splitmix64(&st);
    for (int f = 0; f < 8; ++f) zob_ep[f] = splitmix64(&st);
}

static int piece_index_from_piece_value(int8_t piece)
{
    if (piece == 0) return -1;
    int absval = piece > 0 ? piece : -piece;
    int color = piece > 0 ? 0 : 1;
    if (absval < 1 || absval > 6) return -1;
    return color * 6 + (absval - 1);
}

uint64_t position_hash(const Position *pos)
{
    uint64_t h = 0;
    for (int sq = 0; sq < 64; ++sq) {
        int8_t v = pos->board[sq];
        if (v == 0) continue;
        int idx = piece_index_from_piece_value(v);
        if (idx >= 0) h ^= zob_piece[sq][idx];
    }

    if (pos->side_to_move == COLOR_WHITE) {
        h ^= zob_side;
    }

    h ^= zob_castle[(int)(pos->castling & 0xF)];

    if (pos->en_passant != POS_NO_SQUARE) {
        int file = pos->en_passant & 7; /* assume 0..7 encoding */
        if (file >= 0 && file < 8) h ^= zob_ep[file];
    }

    return h;
}

/* ------------------------------------------------------------------
   Zobrist incremental helper functions (externally visible)
   These must be non-static so movegen.c can call them and the linker
   will find them in this object file.
   ------------------------------------------------------------------ */

/* XOR the zobrist key for the given piece on square 'sq' into *h.
   piece is the same int8_t encoding used in Position->board (0 means empty). */
void zobrist_xor_piece(uint64_t *h, int sq, int8_t piece)
{
    int idx = piece_index_from_piece_value(piece);
    if (idx >= 0 && sq >= 0 && sq < 64) {
        *h ^= zob_piece[sq][idx];
    }
}

/* XOR the side-to-move key */
void zobrist_xor_side(uint64_t *h)
{
    *h ^= zob_side;
}

/* XOR castling key for a castling mask (0..15) */
void zobrist_xor_castle(uint64_t *h, int castling_mask)
{
    *h ^= zob_castle[castling_mask & 0xF];
}

/* XOR en-passant key for an en-passant square (POS_NO_SQUARE means no-op).
   enp_sq should encode file as (sq & 7). */
void zobrist_xor_ep(uint64_t *h, int enp_sq)
{
    if (enp_sq != POS_NO_SQUARE) {
        int file = enp_sq & 7;
        if (file >= 0 && file < 8) *h ^= zob_ep[file];
    }
}
