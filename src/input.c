#include "input.h"
#include <orbis/Pad.h>
#include <orbis/UserService.h>
#include <stdio.h>

static int g_pad = -1;
static uint32_t g_prev = 0;
static uint32_t g_cur  = 0;
static uint32_t g_pressed = 0;
static int g_changed = 0;

int input_init(void) {
    int user = 0;
    int rc = sceUserServiceGetInitialUser(&user);
    if (rc < 0) { printf("[input] no user %d\n", rc); return -1; }
    g_pad = scePadOpen(user, ORBIS_PAD_PORT_TYPE_STANDARD, 0, NULL);
    if (g_pad < 0) { printf("[input] pad open failed %d\n", g_pad); return -1; }
    return 0;
}

void input_shutdown(void) {
    if (g_pad >= 0) scePadClose(g_pad);
}

int input_poll(void) {
    if (g_pad < 0) return 0;
    OrbisPadData d;
    if (scePadReadState(g_pad, &d) < 0) return 0;
    g_prev = g_cur;
    g_cur = d.buttons;
    g_pressed = g_cur & ~g_prev;
    g_changed = (g_cur != g_prev);
    return g_changed;
}

int input_pressed_up(void)     { return (g_pressed & ORBIS_PAD_BUTTON_UP)     ? 1 : 0; }
int input_pressed_down(void)   { return (g_pressed & ORBIS_PAD_BUTTON_DOWN)   ? 1 : 0; }
int input_pressed_cross(void)  { return (g_pressed & ORBIS_PAD_BUTTON_CROSS)  ? 1 : 0; }
int input_pressed_circle(void) { return (g_pressed & ORBIS_PAD_BUTTON_CIRCLE) ? 1 : 0; }
