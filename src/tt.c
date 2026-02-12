#include "tt.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>


typedef struct {
    uint64_t key;
    int32_t value;
    int16_t from;
    int16_t to;
    int8_t promo;
    int8_t depth;
    uint8_t flag;
} TTEntry;

static TTEntry *tt_table = NULL;
static size_t tt_count = 0;

void tt_init(size_t size_mb)
{
    if (tt_table) return;
    if (size_mb == 0) return;
    size_t bytes = size_mb * 1024ULL * 1024ULL;
    tt_count = bytes / sizeof(TTEntry);
    if (tt_count < 1) tt_count = 1;
    tt_table = calloc(tt_count, sizeof(TTEntry));
}

void tt_free(void)
{
    free(tt_table);
    tt_table = NULL;
    tt_count = 0;
}

static inline TTEntry *entry_for_key(uint64_t key)
{
    if (!tt_table || tt_count == 0) return NULL;
    size_t idx = (size_t)(key % tt_count);
    return &tt_table[idx];
}

int tt_probe(uint64_t key, int depth, int alpha, int beta,
             int *out_value, int *out_from, int *out_to, int *out_promo)
{
    TTEntry *e = entry_for_key(key);
    if (!e) return 0;
    if (e->key != key) return 0;
    if ((int)e->depth < depth) {
        if (out_from) *out_from = e->from;
        if (out_to) *out_to = e->to;
        if (out_promo) *out_promo = e->promo;
        return 0;
    }

    int val = e->value;
    if (e->flag == TT_FLAG_EXACT) {
        if (out_value) *out_value = val;
        if (out_from) *out_from = e->from;
        if (out_to) *out_to = e->to;
        if (out_promo) *out_promo = e->promo;
        return 1;
    } else if (e->flag == TT_FLAG_LOWER) {
        if (val >= beta) {
            if (out_value) *out_value = val;
            if (out_from) *out_from = e->from;
            if (out_to) *out_to = e->to;
            if (out_promo) *out_promo = e->promo;
            return 1;
        }
    } else if (e->flag == TT_FLAG_UPPER) {
        if (val <= alpha) {
            if (out_value) *out_value = val;
            if (out_from) *out_from = e->from;
            if (out_to) *out_to = e->to;
            if (out_promo) *out_promo = e->promo;
            return 1;
        }
    }
    if (out_from) *out_from = e->from;
    if (out_to) *out_to = e->to;
    if (out_promo) *out_promo = e->promo;
    return 0;
}

void tt_store(uint64_t key, int value, int depth, tt_flag_t flag,
              int from, int to, int promo)
{
    TTEntry *e = entry_for_key(key);
    if (!e) return;
    if (e->key == 0 || depth >= e->depth) {
        e->key = key;
        e->value = value;
        e->from = (int16_t)from;
        e->to = (int16_t)to;
        e->promo = (int8_t)promo;
        e->depth = (int8_t)depth;
        e->flag = (uint8_t)flag;
    }
}
