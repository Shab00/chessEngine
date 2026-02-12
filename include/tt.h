#ifndef CHESS_TT_H
#define CHESS_TT_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { TT_FLAG_EXACT = 0, TT_FLAG_LOWER = 1, TT_FLAG_UPPER = 2 } tt_flag_t;

void tt_init(size_t size_mb);

int tt_probe(uint64_t key, int depth, int alpha, int beta,
             int *out_value, int *out_from, int *out_to, int *out_promo);

void tt_store(uint64_t key, int value, int depth, tt_flag_t flag,
              int from, int to, int promo);

void tt_free(void);

#ifdef __cplusplus
}
#endif

#endif
