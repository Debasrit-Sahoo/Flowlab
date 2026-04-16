#ifndef CONFIG_LOADER_H
#define CONFIG_LOADER_H

#include <stdint.h>

int config_load(const char *path, uint8_t* port_used);

#endif