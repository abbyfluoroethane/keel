/* Laptop (SDL2) backend internals. Nothing in ui/ includes this. */
#ifndef KEEL_PLATFORM_SDL_H
#define KEEL_PLATFORM_SDL_H

#include <stdbool.h>
#include <stdint.h>

#include "lvgl.h"

/* Open the window (KEEL_HOR_RES x KEEL_VER_RES times `scale`) and create
 * the LVGL display, mouse pointer and keypad. */
lv_display_t * platform_sdl_init(int scale);

/* Run LVGL for `ms` milliseconds of wall time. */
void platform_pump(uint32_t ms);

/* Write the display's current frame as an RGB PNG. */
bool platform_write_png(lv_display_t * disp, const char * path);

/* Scripted swipes, scrolls and button presses; prints frame statistics.
 * Returns 0 when every scenario holds 60 fps (see report() in bench.c). */
int platform_bench(lv_display_t * disp);

#endif /* KEEL_PLATFORM_SDL_H */
