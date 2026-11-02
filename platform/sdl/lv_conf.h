/* LVGL configuration for the laptop (SDL2) build of Keel.
 *
 * Only settings that differ from LVGL's defaults are listed here.
 * lv_conf_internal.h fills in the rest.
 *
 * Rule for this file: nothing here may give the laptop a draw path the
 * device lacks. The device (RK3506B, no GPU) renders with LVGL's software
 * renderer into a DRM/fbdev buffer, so the laptop does the same and SDL
 * only copies the finished frame to the window.
 */
#ifndef LV_CONF_H
#define LV_CONF_H

/* 32-bit color matches an XRGB8888 DRM buffer on the device. */
#define LV_COLOR_FORMAT_DEFAULT LV_COLOR_FORMAT_XRGB8888

/* Memory: system malloc. The device has 512MB, so LVGL's 64KB pool is
 * the wrong limit to test against. */
#define LV_USE_STDLIB_MALLOC  LV_STDLIB_CLIB
#define LV_USE_STDLIB_STRING  LV_STDLIB_CLIB
#define LV_USE_STDLIB_SPRINTF LV_STDLIB_CLIB

/* 60 Hz panel. */
#define LV_DEF_REFR_PERIOD 16
#define LV_DPI_DEF 233
/* Read touch at 100 Hz. LVGL's default (33 ms) would cap drags and
 * scrolling at 30 fps no matter how fast the renderer is. */
#define LV_INDEV_DEF_READ_PERIOD 10

/* Software renderer only. No SDL texture renderer, no OpenGL, no SIMD
 * paths the Cortex-A7 build would not also have. */
#define LV_USE_DRAW_SW 1
#define LV_USE_DRAW_SDL 0
#define LV_USE_DRAW_SW_ASM LV_DRAW_SW_ASM_NONE
/* Keel draws no shadows, so the shadow cache is off. */
#define LV_DRAW_SW_SHADOW_CACHE_SIZE 0

#define LV_USE_LOG 1
#define LV_LOG_LEVEL LV_LOG_LEVEL_WARN
#define LV_LOG_PRINTF 1

/* Keel styles every object itself. */
#define LV_USE_THEME_DEFAULT 0
#define LV_USE_THEME_SIMPLE 0
#define LV_USE_THEME_MONO 0

/* Decoded cover art stays in RAM: 10 albums x 4 sizes is about 5MB. */
#define LV_CACHE_DEF_SIZE (8 * 1024 * 1024)
#define LV_IMAGE_HEADER_CACHE_DEF_CNT 64

/* Fonts: IBM Plex through FreeType at runtime. The built-in Montserrat
 * fonts stay off so nothing falls back to them by accident. */
#define LV_FONT_MONTSERRAT_14 0
#define LV_FONT_DEFAULT &lv_font_unscii_8
#define LV_FONT_UNSCII_8 1
#define LV_USE_FREETYPE 1
#define LV_FREETYPE_USE_LVGL_PORT 0
#define LV_FREETYPE_CACHE_FT_GLYPH_CNT 512

/* Files and images. */
#define LV_USE_FS_STDIO 1
#define LV_FS_STDIO_LETTER 'A'
#define LV_FS_STDIO_PATH ""
#define LV_FS_STDIO_CACHE_SIZE 4096
#define LV_USE_LODEPNG 1

/* --screenshot reads the frame back; --fps needs nothing extra. */
#define LV_USE_SNAPSHOT 0
#define LV_USE_SYSMON 0
#define LV_USE_OBSERVER 1

/* Widgets Keel does not use. Fewer widgets, smaller binary. */
#define LV_USE_ANIMIMG 0
#define LV_USE_ARCLABEL 0
#define LV_USE_CALENDAR 0
#define LV_USE_CANVAS 0
#define LV_USE_CHART 0
#define LV_USE_IMAGEBUTTON 0
#define LV_USE_KEYBOARD 0
#define LV_USE_LED 0
#define LV_USE_LIST 0
#define LV_USE_LOTTIE 0
#define LV_USE_MENU 0
#define LV_USE_MSGBOX 0
#define LV_USE_ROLLER 0
#define LV_USE_SCALE 0
#define LV_USE_SPAN 0
#define LV_USE_SPINBOX 0
#define LV_USE_SPINNER 0
#define LV_USE_TABLE 0
#define LV_USE_TABVIEW 0
#define LV_USE_TEXTAREA 0
#define LV_USE_TILEVIEW 0
#define LV_USE_WIN 0
#define LV_USE_3DTEXTURE 0

/* LVGL's SDL driver stays off: platform/sdl/sdl_port.c owns the window
 * and input, and copies dirty rectangles the way the device's DRM flush
 * will. LVGL itself is built with no SDL dependency. */
#define LV_USE_SDL 0

#define LV_BUILD_EXAMPLES 0
#define LV_BUILD_DEMOS 0

#endif /*LV_CONF_H*/
