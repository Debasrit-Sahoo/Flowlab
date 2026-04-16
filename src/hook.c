#define UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include "keybinds.h"
#include "dispatcher.h"
#include <stdio.h>

static uint8_t g_mod_state = 0;
static HHOOK g_hook = NULL;

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode != HC_ACTION) {
        return CallNextHookEx(g_hook, nCode, wParam, lParam);
    }

    KBDLLHOOKSTRUCT *kb = (KBDLLHOOKSTRUCT*)lParam;
    uint32_t vk = kb->vkCode;

    uint8_t is_down = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
    uint8_t is_up   = (wParam == WM_KEYUP   || wParam == WM_SYSKEYUP);

    // -------- 1. UPDATE MODIFIER STATE FIRST --------
    switch (vk) {

        case VK_LCONTROL:
        case VK_RCONTROL:
            if (is_down) g_mod_state |= KBD_MOD_CTRL;
            if (is_up)   g_mod_state &= ~KBD_MOD_CTRL;
            break;

        case VK_LSHIFT:
        case VK_RSHIFT:
            if (is_down) g_mod_state |= KBD_MOD_SHIFT;
            if (is_up)   g_mod_state &= ~KBD_MOD_SHIFT;
            break;

        case VK_LMENU:
        case VK_RMENU:
            if (is_down) g_mod_state |= KBD_MOD_ALT;
            if (is_up)   g_mod_state &= ~KBD_MOD_ALT;
            break;

        case VK_LWIN:
        case VK_RWIN:
            if (is_down) g_mod_state |= KBD_MOD_WIN;
            if (is_up)   g_mod_state &= ~KBD_MOD_WIN;
            break;
    }

    // -------- 2. ONLY PROCESS REAL KEY PRESSES --------
    if (is_down &&
        vk != VK_LCONTROL && vk != VK_RCONTROL &&
        vk != VK_LSHIFT   && vk != VK_RSHIFT   &&
        vk != VK_LMENU    && vk != VK_RMENU    &&
        vk != VK_LWIN     && vk != VK_RWIN)
    {
        uint8_t mods = g_mod_state;

        Rule *r = keybind_lookup(mods, (uint8_t)vk);
        if (r) {
            dispatch_keybind(r);
        }
    }

    return CallNextHookEx(g_hook, nCode, wParam, lParam);
}

void hook_install(void){
    g_hook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
    if (!g_hook){
        printf("hook failed: %lu\n", GetLastError());
    }
}

void hook_uninstall(void){
    if (g_hook){
        UnhookWindowsHookEx(g_hook);
        g_hook = NULL;
    }
}