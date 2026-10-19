#include <orbis/libkernel.h>
#include <orbis/SystemService.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "video.h"
#include "input.h"
#include "http.h"
#include "install.h"
#include "catalog.h"

/* ------- CONFIG: change this to your GitHub catalog URL ------- */
#define CATALOG_URL   "https://raw.githubusercontent.com/SpaceAceIL/cyrene-store/main/catalog.txt"
#define DOWNLOAD_DIR  "/data/cyrene-store"
/* ------------------------------------------------------------- */

static void logf_(const char* fmt, ...) {
    va_list ap; va_start(ap, fmt); vprintf(fmt, ap); va_end(ap);
    fflush(stdout);
}

typedef enum { SCR_LIST, SCR_DETAIL, SCR_DOWNLOADING, SCR_INSTALLING, SCR_RESULT } screen_t;

static catalog_t      g_cat;
static int            g_sel = 0;
static int            g_scroll = 0;
static screen_t       g_scr = SCR_LIST;
static char           g_errbuf[128];
static char           g_dl_path[512];
static size_t         g_dl_received = 0, g_dl_total = 0;
static int            g_result_ok = 0;

static void draw_bar(uint32_t* fb, int x, int y, int w, int h, int pct) {
    video_fill_rect(fb, x, y, w, h, COL_DARKGRAY);
    if (pct > 0) video_fill_rect(fb, x, y, (w * pct) / 100, h, COL_ACCENT);
}

static void draw_frame(const char* status) {
    uint32_t* fb = video_begin_frame();
    video_clear(fb, COL_DARKBG);
    video_fill_rect(fb, 0, 0, SCR_W, 80, COL_TITLEBG);
    video_draw_str(fb, 40, 24, "CYRENE STORE", COL_WHITE, COL_TITLEBG, 4);

    if (g_scr == SCR_LIST) {
        const int ROWS = 12, LINE = 64, TOP = 130;
        for (int r = 0; r < ROWS; r++) {
            int idx = g_scroll + r;
            if (idx >= g_cat.count) break;
            int y = TOP + r * LINE;
            int sel = (idx == g_sel);
            uint32_t fg = sel ? COL_BLACK : COL_WHITE;
            uint32_t bg = sel ? COL_SELECT : COL_DARKBG;
            if (sel) video_fill_rect(fb, 30, y, SCR_W - 60, LINE - 8, COL_SELECT);
            char line[256];
            snprintf(line, sizeof line, "%s %-30s  v%s", sel ? ">" : " ",
                     g_cat.apps[idx].name, g_cat.apps[idx].version);
            video_draw_str(fb, 50, y + 14, line, fg, bg, 3);
        }
        if (g_cat.count == 0) video_draw_str(fb, 50, 200, "No apps in catalog", COL_GRAY, COL_DARKBG, 3);
    } else if (g_scr == SCR_DETAIL) {
        catalog_app_t* a = &g_cat.apps[g_sel];
        video_draw_str(fb, 50, 130, a->name, COL_WHITE, COL_DARKBG, 4);
        char tmp[256];
        snprintf(tmp, sizeof tmp, "Version: %s", a->version);
        video_draw_str(fb, 50, 210, tmp, COL_GRAY, COL_DARKBG, 2);
        snprintf(tmp, sizeof tmp, "Title:   %s", a->title_id);
        video_draw_str(fb, 50, 250, tmp, COL_GRAY, COL_DARKBG, 2);
        video_draw_str(fb, 50, 320, a->description, COL_WHITE, COL_DARKBG, 2);
        video_draw_str(fb, 50, 450, "[X] Download & Install", COL_ACCENT, COL_DARKBG, 2);
        video_draw_str(fb, 50, 490, "[O] Back",               COL_GRAY,   COL_DARKBG, 2);
    } else if (g_scr == SCR_DOWNLOADING) {
        video_draw_str(fb, 50, 200, "Downloading...", COL_WHITE, COL_DARKBG, 3);
        int pct = g_dl_total ? (int)((g_dl_received * 100) / g_dl_total) : 0;
        draw_bar(fb, 50, 300, SCR_W - 100, 40, pct);
        char tmp[64];
        snprintf(tmp, sizeof tmp, "%zu / %zu bytes", g_dl_received, g_dl_total);
        video_draw_str(fb, 50, 360, tmp, COL_GRAY, COL_DARKBG, 2);
    } else if (g_scr == SCR_INSTALLING) {
        video_draw_str(fb, 50, 200, "Installing... (this can take a minute)", COL_WHITE, COL_DARKBG, 3);
    } else if (g_scr == SCR_RESULT) {
        video_draw_str(fb, 50, 200, g_result_ok ? "INSTALLED SUCCESSFULLY" : "INSTALL FAILED",
                       g_result_ok ? COL_ACCENT : COL_WHITE, COL_DARKBG, 3);
        if (!g_result_ok) video_draw_str(fb, 50, 260, g_errbuf, COL_GRAY, COL_DARKBG, 2);
        video_draw_str(fb, 50, 400, "Press X to return", COL_GRAY, COL_DARKBG, 2);
    }

    video_fill_rect(fb, 0, SCR_H - 60, SCR_W, 60, COL_TITLEBG);
    video_draw_str(fb, 40, SCR_H - 44, status, COL_GRAY, COL_TITLEBG, 2);
    video_end_frame();
}

static void do_fetch_catalog(void) {
    draw_frame("Fetching catalog...");
    char* text = NULL; size_t len = 0; const char* err = NULL;
    int rc = http_get_string(CATALOG_URL, &text, &len, &err);
    if (rc != 0) {
        snprintf(g_errbuf, sizeof g_errbuf, "Catalog fetch: %s", err ? err : "unknown");
        logf_("[store] %s\n", g_errbuf);
        g_cat.count = 0; return;
    }
    catalog_parse(text, &g_cat);
    free(text);
    logf_("[store] catalog: %d apps\n", g_cat.count);
}

static void do_download_and_install(void) {
    catalog_app_t* a = &g_cat.apps[g_sel];
    snprintf(g_dl_path, sizeof g_dl_path, "%s/%s-%s.pkg", DOWNLOAD_DIR, a->id, a->version);
    g_dl_received = 0; g_dl_total = 0;
    g_scr = SCR_DOWNLOADING;
    draw_frame("Downloading");

    extern void on_progress(size_t r, size_t t, void* u);
    const char* err = NULL;
    int rc = http_download(a->url, g_dl_path, on_progress, NULL, &err);
    if (rc != 0) {
        snprintf(g_errbuf, sizeof g_errbuf, "Download: %s", err ? err : "unknown");
        g_result_ok = 0; g_scr = SCR_RESULT; return;
    }

    g_scr = SCR_INSTALLING;
    draw_frame("Installing");
    int irc = install_pkg(g_dl_path);
    if (irc < 0) {
        snprintf(g_errbuf, sizeof g_errbuf, "Install: error 0x%08X", (unsigned)irc);
        g_result_ok = 0;
    } else {
        g_result_ok = 1;
    }
    g_scr = SCR_RESULT;
}

void on_progress(size_t r, size_t t, void* u) {
    (void)u;
    g_dl_received = r; g_dl_total = t;
    draw_frame("Downloading");
}

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    logf_("[store] M0 starting\n");
    if (video_init() < 0) return 1;
    if (input_init() < 0) return 1;
    if (install_init() < 0) logf_("[store] install_init failed\n");
    if (http_init() < 0)  logf_("[store] http_init failed\n");

    do_fetch_catalog();

    while (1) {
        input_poll();
        if (input_pressed_circle()) {
            if (g_scr == SCR_LIST) break;
            if (g_scr == SCR_DETAIL) g_scr = SCR_LIST;
            continue;
        }
        if (g_scr == SCR_RESULT) {
            if (input_pressed_cross()) { g_scr = SCR_LIST; draw_frame("Ready"); }
            continue;
        }
        if (g_scr == SCR_LIST) {
            if (input_pressed_up()   && g_sel > 0) g_sel--;
            if (input_pressed_down() && g_sel < g_cat.count - 1) g_sel++;
            if (g_sel < g_scroll) g_scroll = g_sel;
            if (g_sel >= g_scroll + 12) g_scroll = g_sel - 11;
            if (input_pressed_cross() && g_cat.count > 0) g_scr = SCR_DETAIL;
            draw_frame("UP/DOWN: select   CROSS: details   CIRCLE: exit");
        } else if (g_scr == SCR_DETAIL) {
            if (input_pressed_cross()) {
                do_download_and_install();
            }
            draw_frame("CROSS: download+install   CIRCLE: back");
        } else {
            draw_frame("Please wait...");
        }
    }

    http_shutdown();
    input_shutdown();
    video_shutdown();
    logf_("[store] clean exit\n");
    sceSystemServiceLoadExec("exit", NULL);
    return 0;
}
