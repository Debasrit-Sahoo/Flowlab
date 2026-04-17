#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "keybinds.h"
/* ===================== UTIL ===================== */

char* trim(char *s) {
    while (isspace((unsigned char)*s)) s++;
    if (*s == 0) return s;

    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return s;
}

int str_eq_icase(const char *a, const char *b) {
    while (*a && *b) {
        if (toupper((unsigned char)*a) != toupper((unsigned char)*b))
            return 0;
        a++; b++;
    }
    return *a == *b;
}

int parse_u16(const char *s, uint16_t *out) {
    char *end;
    long val = strtol(s, &end, 10);

    if (*end != '\0') return 0;
    if (val < 0 || val > 65535) return 0;

    *out = (uint16_t)val;
    return 1;
}

/* ===================== MODS ===================== */

int parse_keybind(char *input, uint32_t *vk, uint8_t *mods) {
    *mods = 0;
    *vk = 0;

    char *context = NULL;
    char *token = strtok_s(input, "+", &context);
    char *last = NULL;

    while (token) {
        token = trim(token);
        last = token;

        char *next = strtok_s(NULL, "+", &context);

        if (next) {
            if (str_eq_icase(token, "CTRL"))      *mods |= KBD_MOD_CTRL;
            else if (str_eq_icase(token, "ALT"))  *mods |= KBD_MOD_ALT;
            else if (str_eq_icase(token, "SHIFT"))*mods |= KBD_MOD_SHIFT;
            else if (str_eq_icase(token, "WIN"))  *mods |= KBD_MOD_WIN;
            else return 0; // invalid modifier
        }

        token = next;
    }

    // must have a real key
    if (!last || strlen(last) != 1)
        return 0;

    *vk = (uint32_t)toupper((unsigned char)last[0]);
    return 1;
}

/* ===================== FLAGS ===================== */

int parse_protocol(char *s, uint8_t *out) {
    if (str_eq_icase(s, "TCP"))      *out = FLAG_TCP;
    else if (str_eq_icase(s, "UDP")) *out = FLAG_UDP;
    else if (str_eq_icase(s, "ALL")) *out = FLAG_TCP | FLAG_UDP;
    else return 0;
    return 1;
}

int parse_direction(char *s, uint8_t *out) {
    if (str_eq_icase(s, "UL"))       *out = FLAG_UL;
    else if (str_eq_icase(s, "DL")) *out = FLAG_DL;
    else if (str_eq_icase(s, "ALL")) *out = FLAG_UL | FLAG_DL;
    else return 0;
    return 1;
}

int parse_action(char *s, uint8_t *out) {
    if (str_eq_icase(s, "BLOCK")) *out = 1;
    else if (str_eq_icase(s, "LIMIT")) *out = 2;
    else return 0;
    return 1;
}

uint8_t build_flags(uint8_t proto, uint8_t dir, uint8_t action) {
    return proto | dir | ((action & 0xF) << ACTION_SHIFT);
}

/* ===================== RANGE ===================== */

uint32_t pack_range(uint16_t start, uint16_t end) {
    return ((uint32_t)start << 16) | end;
}

/* ===================== MAIN PARSER ===================== */

int parse_line(char *line,
               uint32_t *vk,
               uint8_t *mods,
               uint32_t *range,
               uint8_t *flags)
{
    char *fields[6];
    int count = 0;

    char* context = NULL;
    char *token = strtok_s(line, ",", &context);
    while (token && count < 6) {
        fields[count++] = trim(token);
        token = strtok_s(NULL, ",", &context);
    }

    if (count != 6) return 0;

    /* --- keybind --- */
    char keybuf[64];
    strncpy_s(keybuf, sizeof(keybuf), fields[0], _TRUNCATE);

    if (!parse_keybind(keybuf, vk, mods))
        return 0;

    /* --- ports --- */
    uint16_t start, end;

    if (!parse_u16(fields[1], &start) ||
        !parse_u16(fields[2], &end) ||
        start > end)
        return 0;

    *range = pack_range(start, end);

    /* --- protocol --- */
    uint8_t proto;
    if (!parse_protocol(fields[3], &proto))
        return 0;

    /* --- direction --- */
    uint8_t dir;
    if (!parse_direction(fields[4], &dir))
        return 0;

    /* --- action --- */
    uint8_t action;
    if (!parse_action(fields[5], &action))
        return 0;

    *flags = build_flags(proto, dir, action);

    return 1;
}