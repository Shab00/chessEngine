#ifndef SEARCH_CONTEXT_H
#define SEARCH_CONTEXT_H

#include <stdint.h>
#include <stddef.h>
#include "timing.h"  
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    volatile int stop;

    uint64_t nodes;
    uint64_t max_nodes;   /* 0 = unlimited */

    uint64_t start_time_ms;
    int time_limit_ms;  /* 0 = no limit */

    int depth_limit;

    int best_from;
    int best_to;
    int best_promo;
    int best_score;
} SearchContext;

static inline void search_context_init(SearchContext *ctx, int depth, int time_limit_ms)
{
    ctx->stop           = 0;
    ctx->nodes          = 0;
    ctx->max_nodes      = 0;
    ctx->start_time_ms  = now_ms();
    ctx->time_limit_ms  = time_limit_ms;
    ctx->depth_limit    = depth;
    ctx->best_from      = -1;
    ctx->best_to        = -1;
    ctx->best_promo     = 0;
    ctx->best_score     = 0;
}

static inline int search_context_should_stop(SearchContext *ctx)
{
    if (ctx->stop) return 1;
    if (ctx->max_nodes > 0 && ctx->nodes >= ctx->max_nodes) return 1;
    if (ctx->time_limit_ms > 0) {
        uint64_t now = now_ms();
        if (now - ctx->start_time_ms >= (uint64_t)ctx->time_limit_ms) {
            ctx->stop = 1;
            return 1;
        }
    }
    return 0;
}

#ifdef __cplusplus
}
#endif

#endif /* SEARCH_CONTEXT_H */
