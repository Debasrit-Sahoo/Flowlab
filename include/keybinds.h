#ifndef KEYBINDS_H
#define KEYBINDS_H

#include <stdint.h>

#define KBD_MOD_NONE 0x0
#define KBD_MOD_CTRL 0x1
#define KBD_MOD_ALT 0x2
#define KBD_MOD_SHIFT 0x4
#define KBD_MOD_WIN 0x8

#define FLAG_TCP   (1 << 0)
#define FLAG_UDP   (1 << 1)
#define FLAG_UL    (1 << 2)
#define FLAG_DL    (1 << 3)
#define ACTION_SHIFT 4

#define MAX_BUCKET_ENTRIES 16
#define MAX_RULES 64

typedef struct{
    uint32_t vk;
    uint8_t mods;
    uint8_t rule_index;
} KeybindDef;

typedef struct{
    uint32_t vk;
    uint8_t rule_index;
} KeybindEntry;

typedef struct{
    KeybindEntry entries[MAX_BUCKET_ENTRIES];
    uint8_t count;
    uint8_t _pad[31];
} KeybindBucket;

typedef struct{
    uint32_t port_range;
    uint8_t rule;
    uint8_t active;
    uint8_t _pad[2];
} Rule;

typedef struct{
    KeybindBucket buckets[16];
    Rule rules[MAX_RULES];
    uint8_t rule_count;
} KeybindTable;

extern KeybindTable g_table;

void keybinds_init(void);
Rule* keybind_lookup(uint8_t mods, uint32_t vk);

#endif
