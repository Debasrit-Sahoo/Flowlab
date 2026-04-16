#include "keybinds.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

KeybindTable g_table;

KeybindDef g_defs[MAX_RULES];
uint8_t g_def_count;

static int vk_cmp(const void *a, const void *b) {
    uint32_t va = ((const KeybindEntry *)a)->vk;
    uint32_t vb = ((const KeybindEntry *)b)->vk;

    return (va > vb) - (va < vb);
}

// returns NULL if not found
Rule* keybind_lookup(uint8_t mods, uint32_t vk) {
    KeybindBucket *b = &g_table.buckets[mods & 0xF];

    if (b->count == 0) return NULL;

    if (b->count < 6) {
        // linear scan for small buckets
        for (int i = 0; i < b->count; i++)
            if (b->entries[i].vk == vk)
                return &g_table.rules[b->entries[i].rule_index];
        return NULL;
    }

    KeybindEntry key = { vk, 0 };
    KeybindEntry *found = bsearch(&key, b->entries, b->count,
                                  sizeof(KeybindEntry), vk_cmp);
    return found ? &g_table.rules[found->rule_index] : NULL;
}

void keybinds_init(void){
    memset(&g_table.buckets, 0, sizeof(g_table.buckets));

    int i;
    for (i = 0; i < g_def_count; i++){
        uint8_t mod = g_defs[i].mods & 0xF;
        KeybindBucket* b = &g_table.buckets[mod];

        if (b->count >= MAX_BUCKET_ENTRIES){
            //should never happen unless the user dumps all keybinds into the same mod
            continue;
        }

        b->entries[b->count].vk = g_defs[i].vk;
        b->entries[b->count].rule_index = g_defs[i].rule_index;
        b->count++;
    }

    //sort buckers for bsearch key = vk
    for (i = 0; i < 16; i++){
        KeybindBucket *b = &g_table.buckets[i];
        if (b->count > 1){
            qsort(b->entries, b->count, sizeof(KeybindEntry), vk_cmp);
        }
    }
}