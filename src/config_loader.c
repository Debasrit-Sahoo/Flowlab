#include "config_loader.h"
#include "parser.h"
#include "keybinds.h"
#include "divert.h"

#include <stdio.h>
#include <string.h>

extern KeybindDef g_defs[MAX_RULES];
extern uint8_t g_def_count;

int config_load(const char *path, uint8_t* port_used){
    FILE *fp = fopen(path, "r");
    if (!fp){return 0;}

    char line[256];
    int line_no = 0;

    if (fgets(line, sizeof(line), fp)){
        if (!strchr(line, '\n') && !strchr(line, '\r') && !feof(fp)){
            fclose(fp);
            return 0;
        }
        line[strcspn(line, "\r\n")] = '\0';
        if (strncmp(line, "policy=", 7) == 0){
            const char *val = line + 7;
            if (strcmp(val, "remote") == 0)     port_policy_flag = 1;
            else if (strcmp(val, "local") == 0) port_policy_flag = 0;
            else { fclose(fp); return 0; }
        } else {
            rewind(fp);
        }
    }

    while (fgets(line, sizeof(line), fp)){
        line_no++;

        if (!strchr(line, '\n') && !strchr(line, '\r')){
            if (!feof(fp)){
                fclose(fp);
                return 0;
            }
        }

        line[strcspn(line, "\r\n")] = '\0';

        if (line[0] == '\0'){continue;}

        uint8_t mods, flags;
        uint32_t range, vk;

        if (!parse_line(line, &vk, &mods, &range, &flags)){
            fclose(fp);
            return 0;
        }

        if (g_table.rule_count >= MAX_RULES || g_def_count >= MAX_RULES){
            fclose(fp);
            return 0;
        }

        int idx = g_table.rule_count;

        uint16_t start = range >> 16;
        uint16_t end = range & 0xFFFF;

        uint32_t p;
        for (p = start; p <= end; p++){
            port_used[p] = 1;
        }
        g_table.rules[idx].port_range = range;
        g_table.rules[idx].rule = (uint8_t)flags;
        g_table.rules[idx].active = 0;
        g_table.rule_count++;

        g_defs[g_def_count].vk = vk;
        g_defs[g_def_count].mods = mods;
        g_defs[g_def_count].rule_index = (uint8_t)idx;
        g_def_count++;
    }
    fclose(fp);
    return 1;
}