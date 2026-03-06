#ifndef CHESS_HASH_H
#define CHESS_HASH_H

#include <stdint.h>
#include "position.h"

#ifdef __cplusplus
extern "C" {
#endif

void zobrist_init(uint64_t seed);

uint64_t position_hash(const Position *pos);

void zobrist_xor_piece(uint64_t *h, int sq, int8_t piece);
void zobrist_xor_side(uint64_t *h);
void zobrist_xor_castle(uint64_t *h, int castling_mask);
void zobrist_xor_ep(uint64_t *h, int enp_sq);

#ifdef __cplusplus
}
#endif

#endif
