#pragma once
#include <stddef.h>

#define CAT_MAX_APPS 32
#define CAT_STR_LEN  256

typedef struct {
    char id[64];
    char name[CAT_STR_LEN];
    char version[32];
    char title_id[16];
    char url[512];
    char description[CAT_STR_LEN];
} catalog_app_t;

typedef struct {
    catalog_app_t apps[CAT_MAX_APPS];
    int count;
} catalog_t;

/* Parse catalog text (pipe-delimited: id|name|version|title_id|url|description). */
int catalog_parse(const char* text, catalog_t* out);
