#define _POSIX_C_SOURCE 199309L
#include "timing.h"
#include <stdint.h>
#include <time.h>

uint64_t now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)(ts.tv_sec) * 1000 + ts.tv_nsec / 1000000;
}
