#ifndef CHESS_TT_H
#define CHESS_TT_H

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { TT_FLAG_EXACT = 0, TT_FLAG_LOWER = 1, TT_FLAG_UPPER = 2 } tt_flag_t;

void tt_init(size_t size_mb);
void tt_clear(void);

int tt_probe(uint64_t key, int depth, int alpha, int beta,
             int *out_value, int *out_from, int *out_to, int *out_promo);

void tt_store(uint64_t key, int value, int depth, tt_flag_t flag,
              int from, int to, int promo);

void tt_free(void);

typedef struct {
    uint64_t probes;
    uint64_t hits;
    uint64_t stores;
    uint64_t overwrites;

    uint64_t probe_by_depth[64];
    uint64_t hit_by_depth[64];
    uint64_t store_by_depth[64];
    uint64_t overwrite_by_depth[64];
} tt_stats_t;

void tt_stats_reset(void);

tt_stats_t tt_stats_get(void);

void tt_stats_print(FILE *out);

void tt_instrument_probe(int depth, int hit);
void tt_instrument_store(int depth, int overwrite);

#ifdef __cplusplus
}
#endif

#endif
