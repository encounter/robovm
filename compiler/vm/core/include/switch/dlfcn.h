#pragma once

enum {
    RTLD_LAZY,
    RTLD_NOW,
    RTLD_GLOBAL,
    RTLD_LOCAL,
};

int dlclose(void *handle);
char *dlerror(void);
void *dlopen(const char *path, int flags);
void *dlsym(void *handle, const char *symbol);
