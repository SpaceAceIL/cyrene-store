#include "catalog.h"
#include <ctype.h>
#include <string.h>

static void trim(char* s) {
    char* p = s; while (*p && isspace((unsigned char)*p)) p++;
    if (p != s) memmove(s, p, strlen(p) + 1);
    size_t n = strlen(s);
    while (n > 0 && isspace((unsigned char)s[n-1])) s[--n] = '\0';
}

static void copy_str(char* dst, size_t cap, const char* src, size_t src_len) {
    if (src_len >= cap) src_len = cap - 1;
    memcpy(dst, src, src_len);
    dst[src_len] = '\0';
}

/* Format: id|name|version|title_id|url|description
 * Blank lines and # comments skipped. */
int catalog_parse(const char* text, catalog_t* out) {
    out->count = 0;
    const char* p = text;
    while (*p && out->count < CAT_MAX_APPS) {
        const char* eol = strchr(p, '\n');
        size_t line_len = eol ? (size_t)(eol - p) : strlen(p);
        char line[1024];
        if (line_len >= sizeof line) line_len = sizeof line - 1;
        memcpy(line, p, line_len);
        line[line_len] = '\0';
        if (eol) p = eol + 1; else p += line_len;

        trim(line);
        if (line[0] == '\0' || line[0] == '#') continue;

        const char* fields[6];
        size_t fl[6];
        int nf = 0;
        const char* fp = line;
        while (nf < 6) {
            const char* sep = strchr(fp, '|');
            fields[nf] = fp;
            fl[nf] = sep ? (size_t)(sep - fp) : strlen(fp);
            nf++;
            if (!sep) break;
            fp = sep + 1;
        }
        if (nf < 5) continue;   /* require at least 5 fields */

        catalog_app_t* a = &out->apps[out->count];
        memset(a, 0, sizeof *a);
        copy_str(a->id, sizeof a->id, fields[0], fl[0]);
        copy_str(a->name, sizeof a->name, fields[1], fl[1]);
        copy_str(a->version, sizeof a->version, fields[2], fl[2]);
        copy_str(a->title_id, sizeof a->title_id, fields[3], fl[3]);
        copy_str(a->url, sizeof a->url, fields[4], fl[4]);
        if (nf >= 6) copy_str(a->description, sizeof a->description, fields[5], fl[5]);
        out->count++;
    }
    return out->count;
}
