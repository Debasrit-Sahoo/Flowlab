#include "dispatcher.h"
#include "statetable.h"
#include <stdint.h>
#include <stdio.h>
#include "keybinds.h"

void dispatch_keybind(Rule *r) {
    if (!r) return;

    r->active ^= 1;   // flip

    uint16_t port_start = (uint16_t)(r->port_range >> 16);
    uint16_t port_end   = (uint16_t)(r->port_range & 0xFFFF);

    uint8_t val = r->active ? r->rule : 0;

    for (uint16_t p = port_start; p <= port_end; p++) {
        g_state_table[p] = val;
    }
    printf("rule=0x%02X ports=%u-%u %s %s%s\n",
        r->rule, port_start, port_end,
        r->active ? "ON" : "OFF",
        (r->rule & FLAG_UL) ? "UL" : "",
        (r->rule & FLAG_DL) ? "DL" : "");
}