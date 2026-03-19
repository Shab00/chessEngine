#ifndef CHESS_EVAL_H
#define CHESS_EVAL_H

#include "position.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Evaluate the position from White's perspective (positive = good for White). */
int evaluate(const Position *pos);

#ifdef __cplusplus
}
#endif

#endif
