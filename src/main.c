#include <Windows.h>
#include <stdint.h>
#include <stdio.h>

#include "config_loader.h"
#include "keybinds.h"
#include "hook.h"
#include "statetable.h"
#include "divert.h"

//cl /std:c11 /W4 /O2 /permissive- /I include /I D:\divertengine\WinDivert\include /Fo:build\obj\ /Fe:build\app.exe src\main.c src\hook.c src\keybinds.c src\config_loader.c src\parser.c src\dispatcher.c src\statetable.c src\divert.c src\limiter.c user32.lib /link /LIBPATH:D:\divertengine\WinDivert\x64 WinDivert.lib ws2_32.lib

static DWORD g_main_thread_id;

static BOOL WINAPI console_handler(DWORD signal) {
    if (signal == CTRL_C_EVENT) {
        PostThreadMessage(g_main_thread_id, WM_QUIT, 0, 0);
        return TRUE;
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
    state_table_init(port_used);
    divert_init();
    free(port_used);
    hook_install();

    g_main_thread_id = GetCurrentThreadId();
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