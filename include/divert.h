#ifndef DIVERT_H
#define DIVERT_H

int divert_init(void);
void divert_close(void);
void divert_loop(void);
extern uint8_t port_policy_flag;

#endif