#include "video.h"
#include "font8x8.h"
#include <orbis/VideoOut.h>
#include <orbis/libkernel.h>
#include <string.h>
#include <stdio.h>

static int g_handle = -1;
static uint32_t* g_fb[2] = {0, 0};
static int g_cur = 0;

#define FB_SIZE  (SCR_W * SCR_H * 4)

int video_init(void) {
    g_handle = sceVideoOutOpen(0xFF, ORBIS_VIDEO_OUT_BUS_MAIN, 0, NULL);
    if (g_handle < 0) { printf("[video] open failed %d\n", g_handle); return -1; }

    OrbisVideoOutBufferAttribute attr;
    sceVideoOutSetBufferAttribute(&attr,
        ORBIS_VIDEO_OUT_PIXEL_FORMAT_A8B8G8R8_SRGB,
        ORBIS_VIDEO_OUT_TILING_MODE_LINEAR,
        ORBIS_VIDEO_OUT_ASPECT_RATIO_16_9,
        SCR_W, SCR_H, SCR_W * 4);

    int32_t off = sceKernelAllocateMainDirectMemory(FB_SIZE * 2, 0x200000, ORBIS_KERNEL_WB_ONION, -1);
    if (off < 0) { printf("[video] alloc failed %d\n", off); return -1; }

    void* base = NULL;
    int rc = sceKernelMapDirectMemory(&base, FB_SIZE * 2,
        ORBIS_KERNEL_PROT_CPU_READ | ORBIS_KERNEL_PROT_CPU_WRITE |
        ORBIS_KERNEL_PROT_GPU_READ | ORBIS_KERNEL_PROT_GPU_WRITE,
        0, (off_t)off, 0x200000);
    if (rc < 0) { printf("[video] map failed %d\n", rc); return -1; }

    g_fb[0] = (uint32_t*)base;
    g_fb[1] = (uint32_t*)((char*)base + FB_SIZE);

    void* addrs[2] = { g_fb[0], g_fb[1] };
    rc = sceVideoOutRegisterBuffers(g_handle, 0, addrs, 2, &attr);
    if (rc < 0) { printf("[video] register failed %d\n", rc); return -1; }
    return 0;
}

void video_shutdown(void) {
    if (g_handle >= 0) sceVideoOutClose(g_handle);
}

uint32_t* video_begin_frame(void) {
    return g_fb[g_cur];
}

void video_end_frame(void) {
    sceVideoOutSubmitFlip(g_handle, g_cur, ORBIS_VIDEO_OUT_FLIP_VSYNC, 0);
    sceVideoOutWaitVblank(g_handle);
    g_cur ^= 1;
}

void video_clear(uint32_t* fb, uint32_t color) {
    for (int i = 0; i < SCR_W * SCR_H; i++) fb[i] = color;
}

void video_fill_rect(uint32_t* fb, int x, int y, int w, int h, uint32_t color) {
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > SCR_W) w = SCR_W - x;
    if (y + h > SCR_H) h = SCR_H - y;
    if (w <= 0 || h <= 0) return;
    for (int yy = y; yy < y + h; yy++) {
        uint32_t* row = fb + yy * SCR_W;
        for (int xx = x; xx < x + w; xx++) row[xx] = color;
    }
}

void video_draw_char(uint32_t* fb, int x, int y, char c, uint32_t fg, uint32_t bg, int scale) {
    if (c < 32 || c > 95) {
        if (c >= 96 && c <= 122) c = (char)(c & ~0x20);
        else c = '?';
    }
    const uint8_t* glyph = FONT8X8[c - 32];
    for (int row = 0; row < 8; row++) {
        uint8_t bits = glyph[row];
        for (int col = 0; col < 8; col++) {
            uint32_t color = (bits & (1 << col)) ? fg : bg;
            int px = x + col * scale;
            int py = y + row * scale;
            for (int sy = 0; sy < scale; sy++) {
                if (py + sy < 0 || py + sy >= SCR_H) continue;
                uint32_t* r = fb + (py + sy) * SCR_W;
                for (int sx = 0; sx < scale; sx++) {
                    if (px + sx >= 0 && px + sx < SCR_W) r[px + sx] = color;
                }
            }
        }
    }
}

void video_draw_str(uint32_t* fb, int x, int y, const char* s, uint32_t fg, uint32_t bg, int scale) {
    int cx = x;
    for (int i = 0; s[i]; i++) {
        video_draw_char(fb, cx, y, s[i], fg, bg, scale);
        cx += 8 * scale;
    }
}
