#include <stdio.h>
#include <inttypes.h>
#include "position.h"
#include "hash.h"


uint64_t compute_zobrist_from_position(const Position *pos)
{
    return position_hash(pos);
}

void assert_zobrist_consistent(const Position *pos)
{
#ifdef DEBUG
    uint64_t recomputed = position_hash(pos);
    uint64_t cached = pos->hash;
    if (recomputed != cached) {
        fprintf(stderr,
                "Zobrist mismatch: recomputed=0x%016" PRIx64 " cached=0x%016" PRIx64 "\n",
                recomputed, cached);
        abort();
    }
#else
    (void)pos;
#endif
}
