#include <Windows.h>
#include <stdint.h>
#include <stdio.h>

#include "config_loader.h"
#include "keybinds.h"
#include "hook.h"
#include "statetable.h"
#include "divert.h"

//cl /std:c11 /W4 /O2 /permissive- /I D:\divertengine\WinDivert\include main.c hook.c keybinds.c config_loader.c parser.c dispatcher.c statetable.c divert.c user32.lib /LIBPATH:D:\divertengine\WinDivert\x64 WinDivert.lib

static BOOL WINAPI console_handler(DWORD signal) {
    if (signal == CTRL_C_EVENT) {
        hook_uninstall();
        divert_close();
        state_table_free();
        ExitProcess(0);
    }
    return TRUE;
}

uint8_t* port_used = NULL;

int main(void){
    port_used = calloc(65536, sizeof(uint8_t));

    if (!config_load("config.txt", port_used)) {
        printf("config load failed\n");
        return 1;
    }

    keybinds_init();
    for (int i = 0; i < g_table.rule_count; i++) {
    Rule *r = &g_table.rules[i];
    printf("rule[%d] range=0x%08X flags=0x%02X\n", i, r->port_range, r->rule);
}
    state_table_init(port_used);
    divert_init();
    free(port_used);
    hook_install();

    SetConsoleCtrlHandler(console_handler, TRUE);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)){
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    hook_uninstall();
    divert_close();
    state_table_free();
    return 0;
}