/* Keel on the laptop: an SDL2 window that shows the 480x800 panel.
 *
 *   keel [--scale N] [--fps] [--reduced-motion]
 *        [--screenshot DIR] [--bench]
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <SDL2/SDL.h>

#include "keel.h"
#include "lvgl.h"
#include "platform.h"
#include "theme.h"

static void usage(void)
{
    fprintf(stderr,
            "usage: keel [options]\n"
            "  --scale N          integer window zoom, 1-4 (rendering stays 480x800)\n"
            "  --fps              show frame time in a corner\n"
            "  --reduced-motion   no parallax, stagger or slides\n"
            "  --screenshot DIR   write one PNG per demo screen to DIR and exit\n"
            "  --bench            run scripted swipes, print frame times, exit\n");
}

void platform_pump(uint32_t ms)
{
    uint32_t end = SDL_GetTicks() + ms;
    while((int32_t)(end - SDL_GetTicks()) > 0) {
        uint32_t next = lv_timer_handler();
        SDL_Delay(LV_MIN(LV_MAX(next, 1u), 2u));
    }
}

static int screenshot_all(lv_display_t * disp, const char * dir)
{
    if(mkdir(dir, 0755) != 0 && errno != EEXIST) {
        fprintf(stderr, "keel: cannot create %s: %s\n", dir, strerror(errno));
        return 1;
    }
    int failed = 0;
    for(int i = 0; i < keel_demo_count(); i++) {
        keel_demo_show(i);
        /* Let enter animations, image decoding and the first player
         * tick finish so the shot shows the settled screen. */
        platform_pump(1200);
        lv_obj_invalidate(lv_screen_active());
        lv_refr_now(disp);
        char path[1024];
        snprintf(path, sizeof(path), "%s/%s.png", dir, keel_demo_name(i));
        if(platform_write_png(disp, path)) printf("%s\n", path);
        else failed = 1;
    }
    return failed;
}

int main(int argc, char ** argv)
{
    int scale = 1;
    const char * shot_dir = NULL;
    bool bench = false;
    keel_options_t opts = {0};

    for(int i = 1; i < argc; i++) {
        if(!strcmp(argv[i], "--scale") && i + 1 < argc) scale = atoi(argv[++i]);
        else if(!strcmp(argv[i], "--fps")) opts.show_fps = true;
        else if(!strcmp(argv[i], "--reduced-motion")) opts.reduced_motion = true;
        else if(!strcmp(argv[i], "--screenshot") && i + 1 < argc) shot_dir = argv[++i];
        else if(!strcmp(argv[i], "--bench")) bench = true;
        else {
            usage();
            return !strcmp(argv[i], "--help") ? 0 : 2;
        }
    }
    if(scale < 1 || scale > 4) scale = 1;

    opts.asset_dir = getenv("KEEL_ASSETS");
    if(!opts.asset_dir) opts.asset_dir = KEEL_ASSET_DIR;

    /* Screenshots need no window on the desktop. */
    if(shot_dir && !getenv("SDL_VIDEODRIVER")) setenv("SDL_VIDEODRIVER", "offscreen", 1);

    lv_init();
    lv_display_t * disp = platform_sdl_init(scale);
    if(!disp) {
        fprintf(stderr, "keel: cannot open the SDL window: %s\n", SDL_GetError());
        return 1;
    }

    if(!keel_ui_init(&opts)) {
        fprintf(stderr, "keel: UI init failed (assets in %s?)\n", opts.asset_dir);
        return 1;
    }

    if(shot_dir) return screenshot_all(disp, shot_dir);
    if(bench) return platform_bench(disp);

    while(1) {
        uint32_t next = lv_timer_handler();
        SDL_Delay(LV_MIN(LV_MAX(next, 1u), 5u));
    }
    return 0;
}
