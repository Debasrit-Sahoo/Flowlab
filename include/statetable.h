#ifndef STATETABLE_H
#define STATETABLE_H

#include <stdint.h>

#define PORTS 65536

extern uint8_t *g_state_table;

void state_table_init(const uint8_t *port_used);
void state_table_free(void);

#endif