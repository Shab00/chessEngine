#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "position.h"
#include "search_context.h"
#include "timing.h"
#include "search.h"

extern uint64_t now_ms(void);

typedef struct {
    const char *fen;
    const char *desc;
} TimingTest;

TimingTest tests[] = {
    {"rn1qkbnr/ppp2ppp/4p3/3p4/3P4/5NP1/PPP1PP1P/RNBQKB1R w KQkq - 0 5", "Balanced midgame"},
    {"4r3/1pp2kpp/p1np4/4q3/1PP5/P2Q4/5PPP/3R2K1 w - - 0 1", "High-mobility endgame"},
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", "Opening/starting"},
};

const size_t NUM_TESTS = sizeof(tests) / sizeof(tests[0]);
const int depths[] = {5, 6};
const size_t NUM_DEPTHS = sizeof(depths) / sizeof(depths[0]);
const uint64_t time_limits[] = {50, 200, 1000, 2000}; // ms
const size_t NUM_LIMITS = sizeof(time_limits) / sizeof(time_limits[0]);

int main(void) {
    for (size_t t = 0; t < NUM_TESTS; ++t) {
        Position pos;
        char err[256];
        if (position_from_fen(&pos, tests[t].fen, err, sizeof(err)) != POS_OK) {
            printf("Test \"%s\": FEN error: %s\n", tests[t].desc, err);
            continue;
        }
        for (size_t d = 0; d < NUM_DEPTHS; ++d) {
            for (size_t l = 0; l < NUM_LIMITS; ++l) {
                int from = -1, to = -1, promo = 0;
                SearchContext ctx;
                search_context_init(&ctx, depths[d], time_limits[l]);
                uint64_t start = now_ms();
                search_root(&pos, depths[d], &from, &to, &promo, &ctx);
                uint64_t elapsed = now_ms() - start;
                printf("Position: %-25s | depth: %2d | limit: %4llums | elapsed: %4llums\n",
                       tests[t].desc, depths[d],
                       (unsigned long long)time_limits[l],
                       (unsigned long long)elapsed);
            }
        }
    }
    return 0;
}
