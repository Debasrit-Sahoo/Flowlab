#include "limiter.h"
#include "keybinds.h"
#include <windows.h>
#include <stdint.h>

#define DEFAULT_RATE_BPS 1
#define SCALE (1ULL << 20)  // fixed point precision, avoids float

RateLimiter g_limiters[MAX_RULES];
uint8_t g_limiter_count = 0;

void limiters_init(void) {
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);

    for (int i = 0; i < g_table.rule_count; i++) {
        if ((g_table.rules[i].rule >> ACTION_SHIFT) != 2) continue;

        uint16_t s = (uint16_t)(g_table.rules[i].port_range >> 16);
        uint16_t e = (uint16_t)(g_table.rules[i].port_range & 0xFFFF);

        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);

        // prescale: rate_per_tick = (rate_bps * SCALE) / freq
        // hot path becomes: tokens += (elapsed * rate_per_tick) >> SCALE_SHIFT
        uint64_t rate_per_tick = (DEFAULT_RATE_BPS * SCALE) / (uint64_t)freq.QuadPart;

        g_limiters[g_limiter_count++] = (RateLimiter){
            .port_start   = s,
            .port_end     = e,
            .tokens       = 0,
            .last_refill  = (uint64_t)now.QuadPart,
            .rate_per_tick = rate_per_tick,
            .max_tokens   = DEFAULT_RATE_BPS * SCALE  // 1 second worth of tokens
        };
    }
}

int limiter_consume(RateLimiter *l, uint32_t packet_len) {
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);

    uint64_t elapsed = (uint64_t)now.QuadPart - l->last_refill;
    l->last_refill   = (uint64_t)now.QuadPart;

    // refill
    uint64_t earned = elapsed * l->rate_per_tick;  // still scaled
    l->tokens += earned;
    if (l->tokens > l->max_tokens) l->tokens = l->max_tokens;

    // packet_len needs same scale to compare
    uint64_t cost = (uint64_t)packet_len * SCALE;
    if (l->tokens >= cost) {
        l->tokens -= cost;
        return 1;  // forward
    }
    return 0;  // drop
}