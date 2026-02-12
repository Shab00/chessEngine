#ifndef CHESS_HASH_H
#define CHESS_HASH_H

#include <stdint.h>
#include "position.h"

#ifdef __cplusplus
extern "C" {
#endif

void zobrist_init(uint64_t seed);

uint64_t position_hash(const Position *pos);

#ifdef __cplusplus
}
#endif

#endif
