// BIT REPR => 8bits 76543210
/*
is_tcp?
is_udp?
is_ul?
is_dl?
4567 -> action table mapping for fx
table[int] -> Pfx(packet)
if (!4567){reinject(packet)}
FOR SANITY, LAST PRESSED KEYBIND IS VALID RULE AND 
UNTOGGLE JUST ZEROES THE ACTION ENTRY TO EFFECTIVELY INVALIDATE THE ENTRY AND EACH KEYPRESS OVERWRITE ENTRY
*/
#include "statetable.h"
#include <stdlib.h>
#include <stdint.h>

uint8_t *g_state_table = NULL;

void state_table_init(const uint8_t *port_used) {
    g_state_table = malloc(PORTS * sizeof(uint8_t));
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