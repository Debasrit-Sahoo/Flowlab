#ifndef LIMITER_H
#define LIMITER_H

#include <stdint.h>
#include "keybinds.h"

typedef struct {
    uint16_t port_start;
    uint16_t port_end;
    uint64_t tokens;
    uint64_t last_refill;  // QPC ticks
    uint64_t rate_per_tick; // bytes per tick, prescaled
    uint64_t max_tokens;    // cap to prevent burst accumulation
} RateLimiter;

extern RateLimiter g_limiters[MAX_RULES];
extern uint8_t g_limiter_count;

void limiters_init(void);
int  limiter_consume(RateLimiter *l, uint32_t packet_len);

#endif