
#include <stdio.h>
#include <stdlib.h>

#include "include/main.h"
#include "include/util.h"

void die_func(const char* func, const char* fmt, ...) {
    va_list va;
    va_start(va, fmt);

    fprintf(stderr, "hl-png: %s: ", func);

    vfprintf(stderr, fmt, va);
    putc('\n', stderr);

    if (g_window != NULL)
        SDL_DestroyWindow(g_window);

    SDL_Quit();
    exit(1);
}
