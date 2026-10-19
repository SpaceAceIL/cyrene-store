#pragma once
#include <stddef.h>
#include <stdint.h>

typedef void (*http_progress_cb)(size_t received, size_t total, void* user);

int  http_init(void);
void http_shutdown(void);

/* Fetch entire response body into *out (malloc'd, caller frees).
 * Returns 0 on success, negative on failure, sets *out=NULL and *err to a static message. */
int  http_get_string(const char* url, char** out, size_t* out_len, const char** err);

/* Stream URL to out_path. progress called per chunk (may be NULL).
 * Returns 0 on success, negative on failure. */
int  http_download(const char* url, const char* out_path, http_progress_cb progress, void* user, const char** err);
