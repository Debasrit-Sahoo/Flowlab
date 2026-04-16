#ifndef PARSER_H
#define PARSER_H

#include <stdint.h>

int parse_line(char *line, uint32_t *vk, uint8_t *mods, uint32_t *range, uint8_t *flags);
uint8_t parse_keybind(char *input, uint32_t *vk, uint8_t *mods);

#endif