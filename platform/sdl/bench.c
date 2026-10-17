/* --bench: scripted one-finger input through a virtual pointer, the same
 * path a touch panel uses. Each scenario prints frame statistics. */
#include <stdio.h>
#include <string.h>

#include <SDL2/SDL.h>

#include "keel.h"
#include "nav.h"
#include "platform.h"
#include "theme.h"

static lv_point_t finger;
static bool finger_down;

static void bench_read(lv_indev_t * indev, lv_indev_data_t * data)
{
    data->point = finger;
    data->state = finger_down ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

/* Drag in a straight line over `ms`, then lift and let things settle. */
static void drag(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t ms, uint32_t settle)
{
    finger = (lv_point_t){x0, y0};
    finger_down = true;
    platform_pump(40);
    uint32_t start = SDL_GetTicks();
    for(;;) {
        uint32_t t = SDL_GetTicks() - start;
        if(t > ms) t = ms;
        finger.x = x0 + (x1 - x0) * (int32_t)t / (int32_t)ms;
        finger.y = y0 + (y1 - y0) * (int32_t)t / (int32_t)ms;
        platform_pump(4);
        if(t >= ms) break;
    }
    finger_down = false;
    platform_pump(settle);
}

static int find_demo(const char * name)
{
    for(int i = 0; i < keel_demo_count(); i++)
        if(!strcmp(keel_demo_name(i), name)) return i;
    return 0;
}

static uint32_t key_now;
static bool key_down;

static void bench_keys(lv_indev_t * indev, lv_indev_data_t * data)
{
    data->key = key_now;
    data->state = key_down ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

static void press(uint32_t key)
{
    key_now = key;
    key_down = true;
    platform_pump(60);
    key_down = false;
    platform_pump(300);
}

/* Vela's buttons: back, home and menu from three screens deep. */
static int check_buttons(lv_display_t * disp)
{
    lv_indev_t * keys = lv_indev_create();
    lv_indev_set_type(keys, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(keys, bench_keys);
    lv_indev_set_display(keys, disp);

    keel_demo_show(find_demo("06-album")); /* home > music > album */
    platform_pump(400);
    int d0 = keel_nav_depth();
    press(KEEL_KEY_BACK);
    int d1 = keel_nav_depth();
    press(KEEL_KEY_MENU);
    int d2 = keel_nav_depth();
    press(KEEL_KEY_HOME);
    int d3 = keel_nav_depth();
    lv_indev_delete(keys);
    bool ok = d0 == 3 && d1 == 2 && d2 == 2 && d3 == 1;
    printf("%-26s depth %d, back %d, menu %d, home %d  %s\n", "buttons", d0, d1, d2, d3, ok ? "ok" : "FAIL");
    return ok ? 0 : 1;
}

/* Pass: 99% of frames inside one 60 Hz period and under 1% over it. A
 * laptop at a balanced power profile throws the odd long frame for reasons
 * outside Keel (the max column shows it); a stall Keel causes repeats. */
static int report(const char * name)
{
    keel_frame_stats_t s;
    keel_frame_stats_get(&s);
    bool ok = s.frames > 0 && s.p99_ms < 16.7 && s.over_budget * 100 < s.frames;
    printf("%-24s frames %4u  avg %5.2f ms (flush %4.2f)  p95 %5.2f  p99 %5.2f  max %5.2f  over %u  %s\n", name,
           s.frames, s.avg_ms, s.avg_flush_ms, s.p95_ms, s.p99_ms, s.max_ms, s.over_budget, ok ? "ok" : "SLOW");
    return ok ? 0 : 1;
}

int platform_bench(lv_display_t * disp)
{
    lv_indev_t * pointer = lv_indev_create();
    lv_indev_set_type(pointer, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(pointer, bench_read);
    lv_indev_set_display(pointer, disp);

    int32_t cx = KEEL_HOR_RES / 2;
    int slow = 0;

    /* Pivot swipes: artists -> playlists and back. */
    keel_demo_show(find_demo("02-music-artists"));
    platform_pump(800);
    keel_frame_stats_reset();
    int32_t panes[6];
    for(int i = 0; i < 3; i++) {
        drag(420, 500, 60, 500, 180, 500);
        panes[i] = *keel_nav_state();
    }
    for(int i = 0; i < 3; i++) {
        drag(60, 500, 420, 500, 180, 500);
        panes[3 + i] = *keel_nav_state();
    }
    slow |= report("pivot swipes");
    /* The swipes must actually move: artists > albums > songs > playlists and back. */
    static const int32_t want[6] = {1, 2, 3, 2, 1, 0};
    bool moved = !memcmp(panes, want, sizeof(want));
    printf("%-26s panes %d %d %d %d %d %d  %s\n", "pivot pane sequence", (int)panes[0], (int)panes[1],
           (int)panes[2], (int)panes[3], (int)panes[4], (int)panes[5], moved ? "ok" : "FAIL");
    slow |= !moved;

    /* Long list: fling the songs pane down and back up. */
    keel_demo_show(find_demo("04-music-songs"));
    platform_pump(800);
    keel_frame_stats_reset();
    for(int i = 0; i < 3; i++) drag(cx, 700, cx, 250, 200, 900);
    for(int i = 0; i < 3; i++) drag(cx, 250, cx, 700, 200, 900);
    slow |= report("songs scroll (fling)");

    /* Slow drag: the finger stays down, every frame follows it. */
    keel_frame_stats_reset();
    drag(cx, 700, cx, 200, 1500, 300);
    drag(cx, 200, cx, 700, 1500, 300);
    slow |= report("songs scroll (slow drag)");

    /* Album grid: 2-column cover art, the most pixels per frame. */
    keel_demo_show(find_demo("03-music-albums"));
    platform_pump(800);
    keel_frame_stats_reset();
    /* x on a tile, not in the gap between the columns. */
    int32_t tile_x = keel_space.margin + KEEL_ART_TILE / 2;
    for(int i = 0; i < 2; i++) drag(tile_x, 700, tile_x, 250, 200, 900);
    for(int i = 0; i < 2; i++) drag(tile_x, 250, tile_x, 700, 200, 900);
    slow |= report("album grid scroll");
    /* Again, as a guard: an earlier build had a one-off slow frame on the
     * first pass only (see README). */
    keel_frame_stats_reset();
    for(int i = 0; i < 2; i++) drag(tile_x, 700, tile_x, 250, 200, 900);
    for(int i = 0; i < 2; i++) drag(tile_x, 250, tile_x, 700, 200, 900);
    slow |= report("album grid scroll, again");

    /* Album screen: large art plus track list. */
    keel_demo_show(find_demo("06-album"));
    platform_pump(800);
    keel_frame_stats_reset();
    drag(cx, 700, cx, 150, 250, 900);
    drag(cx, 150, cx, 700, 250, 900);
    slow |= report("album screen scroll");

    /* Now playing: swipe the art to skip tracks. */
    keel_demo_show(find_demo("07-now-playing"));
    platform_pump(800);
    keel_frame_stats_reset();
    for(int i = 0; i < 3; i++) drag(420, 300, 60, 300, 180, 600);
    slow |= report("now playing skip swipes");

    /* Radio: drag the tuning strip both ways (y is the strip's middle). */
    keel_demo_show(find_demo("08-radio"));
    platform_pump(800);
    keel_frame_stats_reset();
    drag(400, 395, 80, 395, 600, 600);
    drag(80, 395, 400, 395, 600, 600);
    slow |= report("radio tuning drag");

    /* Screen opens: the row stagger animation. */
    keel_frame_stats_reset();
    for(int i = 0; i < 3; i++) {
        keel_demo_show(find_demo("09-settings"));
        platform_pump(600);
    }
    slow |= report("screen open (stagger)");

    slow |= check_buttons(disp);

    printf(slow ? "bench: FAIL\n" : "bench: all scenarios hold 60 fps\n");
    return slow;
}
