#include "statetable.h"
#include <stdlib.h>
#include <stdint.h>

uint8_t *g_state_table = NULL;

void state_table_init(const uint8_t *port_used) {
    g_state_table = calloc(PORTS, sizeof(uint8_t));
    if (!g_state_table) return;

    for (int i = 0; i < PORTS; i++) {
        if (port_used[i]) {
            g_state_table[i] = 0;
        }
    }
}

void state_table_free(void) {
    free(g_state_table);
    g_state_table = NULL;
}