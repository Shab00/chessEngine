#ifndef CHESS_SEARCH_H
#define CHESS_SEARCH_H

#include "position.h"

#ifdef __cplusplus
extern "C" {
#endif

int search_root(Position *pos, int depth, int *out_from, int *out_to, int *out_promotion);

int search_iterative_deepening(Position *pos, int max_depth, int *out_from, int *out_to, int *out_promotion);

void search_init(int max_depth);
void search_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif
