#pragma once
#include <stdint.h>

int  input_init(void);
void input_shutdown(void);

/* Poll current state. Returns 1 if state changed, 0 if same as last frame. */
int  input_poll(void);

/* Edge-triggered: pressed this frame only */
int  input_pressed_up(void);
int  input_pressed_down(void);
int  input_pressed_cross(void);
int  input_pressed_circle(void);
