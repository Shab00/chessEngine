#ifndef CHESS_SEARCH_H
#define CHESS_SEARCH_H

#include "position.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Search API:
   - search_root: run fixed-depth alpha-beta and return best move via pointers.
     Returns 1 if a move was found, 0 if no legal moves.
*/
int search_root(Position *pos, int depth, int *out_from, int *out_to, int *out_promotion);

#ifdef __cplusplus
}
#endif

#endif
