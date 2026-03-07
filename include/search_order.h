#ifndef SEARCH_ORDER_H
#define SEARCH_ORDER_H

#include <stdint.h>
#include "position.h"


#define KILLER_SLOTS 2

void order_init(int max_depth);
void order_free(void);

int score_move_from(const Position *pos, int from, int to, int promo, int ply);

void score_moves_from(const Position *pos,
                      const int *froms, const int *tos, const int *promos,
                      int n_moves, int ply, int *out_scores);

void update_killers_from(int ply, int from, int to, int promo);

void update_history_from(const Position *pos, int from, int to, int promo, int depth);

#endif 
