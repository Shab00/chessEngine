#ifndef CHESS_SEARCH_H
#define CHESS_SEARCH_H

#include "position.h"
#include "search_context.h"

#ifdef __cplusplus
extern "C" {
#endif

int search_root(Position *pos, int depth, int *out_from, int *out_to, int *out_promotion, SearchContext *ctx);

#ifdef __cplusplus
}
#endif

#endif /* CHESS_SEARCH_H */
