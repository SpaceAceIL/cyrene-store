#pragma once
#include <stdint.h>

int  video_init(void);
void video_shutdown(void);
uint32_t* video_begin_frame(void);
void video_end_frame(void);

void video_clear(uint32_t* fb, uint32_t color);
void video_fill_rect(uint32_t* fb, int x, int y, int w, int h, uint32_t color);
void video_draw_char(uint32_t* fb, int x, int y, char c, uint32_t fg, uint32_t bg, int scale);
void video_draw_str(uint32_t* fb, int x, int y, const char* s, uint32_t fg, uint32_t bg, int scale);

/* A8B8G8R8 LE layout: R=low byte, G, B, A=high byte. */
#define RGB(r,g,b)   ((uint32_t)(0xFF000000u | ((uint32_t)(b)<<16) | ((uint32_t)(g)<<8) | (uint32_t)(r)))
#define COL_BLACK       RGB(0,   0,   0)
#define COL_WHITE       RGB(255, 255, 255)
#define COL_DARKBG      RGB(16,  24,  32)
#define COL_TITLEBG     RGB(31,  64,  104)
#define COL_ACCENT      RGB(63,  160, 255)
#define COL_SELECT      RGB(63,  160, 255)
#define COL_GRAY        RGB(170, 170, 170)
#define COL_DARKGRAY    RGB(85,  85,  85)

#define SCR_W 1920
#define SCR_H 1080
