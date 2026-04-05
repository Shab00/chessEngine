#include "tt.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdatomic.h>
#include <inttypes.h>
#include <stdio.h>

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

static _Atomic uint64_t tt_probes = 0;
static _Atomic uint64_t tt_hits = 0;
static _Atomic uint64_t tt_stores = 0;
static _Atomic uint64_t tt_overwrites = 0;

static _Atomic uint64_t tt_probe_by_depth[64];
static _Atomic uint64_t tt_hit_by_depth[64];
static _Atomic uint64_t tt_store_by_depth[64];
static _Atomic uint64_t tt_overwrite_by_depth[64];

void tt_stats_reset(void)
{
    atomic_store(&tt_probes, 0);
    atomic_store(&tt_hits, 0);
    atomic_store(&tt_stores, 0);
    atomic_store(&tt_overwrites, 0);
    for (int i = 0; i < 64; ++i) {
        atomic_store(&tt_probe_by_depth[i], 0);
        atomic_store(&tt_hit_by_depth[i], 0);
        atomic_store(&tt_store_by_depth[i], 0);
        atomic_store(&tt_overwrite_by_depth[i], 0);
    }
}

tt_stats_t tt_stats_get(void)
{
    tt_stats_t s;
    s.probes = (uint64_t)atomic_load(&tt_probes);
    s.hits = (uint64_t)atomic_load(&tt_hits);
    s.stores = (uint64_t)atomic_load(&tt_stores);
    s.overwrites = (uint64_t)atomic_load(&tt_overwrites);
    for (int i = 0; i < 64; ++i) {
        s.probe_by_depth[i] = (uint64_t)atomic_load(&tt_probe_by_depth[i]);
        s.hit_by_depth[i]   = (uint64_t)atomic_load(&tt_hit_by_depth[i]);
        s.store_by_depth[i] = (uint64_t)atomic_load(&tt_store_by_depth[i]);
        s.overwrite_by_depth[i] = (uint64_t)atomic_load(&tt_overwrite_by_depth[i]);
    }
    return s;
}

void tt_stats_print(FILE *out)
{
    if (!out) out = stdout;
    tt_stats_t s = tt_stats_get();
    fprintf(out, "TT stats: probes=%" PRIu64 " hits=%" PRIu64 " stores=%" PRIu64 " overwrites=%" PRIu64 "\n",
            s.probes, s.hits, s.stores, s.overwrites);

    fprintf(out, "Per-depth (depth: probes hits stores overwrites hit_rate%%)\n");
    for (int d = 0; d < 64; ++d) {
        uint64_t p = s.probe_by_depth[d];
        if (p == 0) continue;
        uint64_t h = s.hit_by_depth[d];
        uint64_t st = s.store_by_depth[d];
        uint64_t ow = s.overwrite_by_depth[d];
        double rate = (p > 0) ? (100.0 * ((double)h / (double)p)) : 0.0;
        fprintf(out, " %2d: %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64 "  %.1f%%\n",
                d, p, h, st, ow, rate);
    }
}

static inline int clamp_depth_bucket(int depth)
{
    if (depth < 0) return 0;
    if (depth >= 64) return 63;
    return depth;
}

void tt_instrument_probe(int depth, int hit)
{
    atomic_fetch_add(&tt_probes, 1);
    int d = clamp_depth_bucket(depth);
    atomic_fetch_add(&tt_probe_by_depth[d], 1);
    if (hit) {
        atomic_fetch_add(&tt_hits, 1);
        atomic_fetch_add(&tt_hit_by_depth[d], 1);
    }
}

void tt_instrument_store(int depth, int overwrite)
{
    atomic_fetch_add(&tt_stores, 1);
    int d = clamp_depth_bucket(depth);
    atomic_fetch_add(&tt_store_by_depth[d], 1);
    if (overwrite) {
        atomic_fetch_add(&tt_overwrites, 1);
        atomic_fetch_add(&tt_overwrite_by_depth[d], 1);
    }
}

void tt_init(size_t size_mb)
{
    if (tt_table) return;
    if (size_mb == 0) return;
    size_t bytes = size_mb * 1024ULL * 1024ULL;
    tt_count = bytes / sizeof(TTEntry);
    if (tt_count < 1) tt_count = 1;
    tt_table = calloc(tt_count, sizeof(TTEntry));
}

void tt_clear(void)
{
    if (tt_table && tt_count > 0)
        memset(tt_table, 0, tt_count * sizeof(TTEntry));
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
    if (!e) {
        tt_instrument_probe(depth, 0);
        return 0;
    }
    if (e->key != key) {
        tt_instrument_probe(depth, 0);
        return 0;
    }

    int usable = 0;
    int val = e->value;

    if ((int)e->depth < depth) {
        if (out_from) *out_from = e->from;
        if (out_to) *out_to = e->to;
        if (out_promo) *out_promo = e->promo;
        tt_instrument_probe(depth, 0);
        return 0;
    }

    if (e->flag == TT_FLAG_EXACT) {
        if (out_value) *out_value = val;
        if (out_from) *out_from = e->from;
        if (out_to) *out_to = e->to;
        if (out_promo) *out_promo = e->promo;
        usable = 1;
    } else if (e->flag == TT_FLAG_LOWER) {
        if (val >= beta) {
            if (out_value) *out_value = val;
            if (out_from) *out_from = e->from;
            if (out_to) *out_to = e->to;
            if (out_promo) *out_promo = e->promo;
            usable = 1;
        }
    } else if (e->flag == TT_FLAG_UPPER) {
        if (val <= alpha) {
            if (out_value) *out_value = val;
            if (out_from) *out_from = e->from;
            if (out_to) *out_to = e->to;
            if (out_promo) *out_promo = e->promo;
            usable = 1;
        }
    }

    if (!usable) {
        if (out_from) *out_from = e->from;
        if (out_to) *out_to = e->to;
        if (out_promo) *out_promo = e->promo;
    }

    tt_instrument_probe(depth, usable ? 1 : 0);
    return usable;
}

void tt_store(uint64_t key, int value, int depth, tt_flag_t flag,
              int from, int to, int promo)
{
    TTEntry *e = entry_for_key(key);
    if (!e) return;

    int overwrite = (e->key != 0) ? 1 : 0;

    if (e->key == 0 || depth >= e->depth) {
        e->key = key;
        e->value = value;
        e->from = (int16_t)from;
        e->to = (int16_t)to;
        e->promo = (int8_t)promo;
        e->depth = (int8_t)depth;
        e->flag = (uint8_t)flag;
        tt_instrument_store(depth, overwrite);
    } else {
        /* no store performed (deeper entry already present) */
    }
}
