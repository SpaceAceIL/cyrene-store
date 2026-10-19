#include <stdbool.h>
#include "http.h"
#include <orbis/Http.h>
#include <orbis/Ssl.h>
#include <orbis/Net.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define POOL_SSL  0x100000
#define POOL_HTTP 0x200000
#define CHUNK     65536

static int g_ready = 0;
static int g_http_ctx = -1;
static int g_ssl_ctx  = -1;

int http_init(void) {
    sceNetInit(0x20000, 0x10, 0x100, 0x10, 0x100);
    g_ssl_ctx = sceSslInit(POOL_SSL);
    if (g_ssl_ctx < 0) return g_ssl_ctx;
    g_http_ctx = sceHttpInit(0, g_ssl_ctx, POOL_HTTP);
    if (g_http_ctx < 0) return g_http_ctx;
    g_ready = 1;
    return 0;
}

void http_shutdown(void) {
    if (g_ready) sceHttpTerm(g_http_ctx);
    if (g_ssl_ctx >= 0) sceSslTerm(g_ssl_ctx);
    g_ready = 0;
}

static int do_get(const char* url, FILE* sink, char** out_buf, size_t* out_len,
                  http_progress_cb cb, void* user, const char** err) {
    if (!g_ready) { *err = "http not initialized"; return -1; }
    int tmpl = sceHttpCreateTemplate(0, "CyreneStore/0.1", 1, 0);
    if (tmpl < 0) { *err = "sceHttpCreateTemplate failed"; return -2; }
    int conn = sceHttpCreateConnectionWithURL(tmpl, url, 1);
    if (conn < 0) { sceHttpDeleteTemplate(tmpl); *err = "sceHttpCreateConnection failed"; return -3; }
    int req = sceHttpCreateRequestWithURL(conn, 0, url, 0);
    if (req < 0) { sceHttpDeleteConnection(conn); sceHttpDeleteTemplate(tmpl); *err = "sceHttpCreateRequest failed"; return -4; }
    int rc = sceHttpSendRequest(req, NULL, 0);
    if (rc < 0) { sceHttpDeleteRequest(req); sceHttpDeleteConnection(conn); sceHttpDeleteTemplate(tmpl); *err = "sceHttpSendRequest failed"; return -5; }
    int status = 0;
    sceHttpGetStatusCode(req, &status);
    if (status != 200) {
        static char sbuf[64]; snprintf(sbuf, sizeof sbuf, "HTTP status %d", status); *err = sbuf;
        sceHttpDeleteRequest(req); sceHttpDeleteConnection(conn); sceHttpDeleteTemplate(tmpl);
        return -6;
    }
    size_t total = 0;
    sceHttpGetResponseContentLength(req, NULL, &total);
    char buf[CHUNK];
    size_t received = 0;
    char* accum = NULL;
    size_t accum_len = 0, accum_cap = 0;
    while (1) {
        int n = sceHttpReadData(req, buf, sizeof buf);
        if (n < 0) { free(accum); *err = "sceHttpReadData failed"; rc = -7; goto done; }
        if (n == 0) break;
        if (sink) fwrite(buf, 1, (size_t)n, sink);
        else {
            if (accum_len + (size_t)n > accum_cap) {
                accum_cap = (accum_cap + (size_t)n) * 2;
                char* np = realloc(accum, accum_cap + 1);
                if (!np) { free(accum); *err = "out of memory"; rc = -8; goto done; }
                accum = np;
            }
            memcpy(accum + accum_len, buf, (size_t)n);
            accum_len += (size_t)n;
        }
        received += (size_t)n;
        if (cb) cb(received, total, user);
    }
    if (out_buf && accum) { accum[accum_len] = '\0'; *out_buf = accum; *out_len = accum_len; }
    else free(accum);
    rc = 0;
done:
    sceHttpDeleteRequest(req); sceHttpDeleteConnection(conn); sceHttpDeleteTemplate(tmpl);
    return rc;
}

int http_get_string(const char* url, char** out, size_t* out_len, const char** err) {
    *out = NULL; if (out_len) *out_len = 0;
    return do_get(url, NULL, out, out_len, NULL, NULL, err);
}

int http_download(const char* url, const char* out_path, http_progress_cb cb, void* user, const char** err) {
    FILE* f = fopen(out_path, "wb");
    if (!f) { *err = "cannot open output file"; return -1; }
    int rc = do_get(url, f, NULL, NULL, cb, user, err);
    fclose(f);
    return rc;
}
