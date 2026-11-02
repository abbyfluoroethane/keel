/* --bench: scripted one-finger input through a virtual pointer, the same
 * path a touch panel uses. Each scenario prints frame statistics. */
#include <stdio.h>
#include <string.h>

#include <SDL2/SDL.h>

#include "keel.h"
#include "library.h"
#include "menu.h"
#include "nav.h"
#include "actions.h"
#include "player.h"
#include "screens.h"
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

static bool pressed_for(keel_button_t b, uint32_t ms)
{
    keel_button_event(b, true);
    platform_pump(ms);
    keel_button_event(b, false);
    platform_pump(300);
    return true;
}

/* Vela's buttons, from three screens deep: back, menu (short opens the
 * screen's menu, back closes it, long opens now playing), home. */
static int check_buttons(void)
{
    keel_demo_show(find_demo("06-album")); /* home > music > album */
    platform_pump(400);
    int d0 = keel_nav_depth();
    pressed_for(KEEL_BUTTON_BACK, 60);
    int d1 = keel_nav_depth();
    keel_nav_push(keel_scr_album, (void *)(intptr_t)3);
    platform_pump(300);
    pressed_for(KEEL_BUTTON_MENU, 60);
    bool menu_short = keel_menu_is_open();
    pressed_for(KEEL_BUTTON_BACK, 60);
    bool menu_closed = !keel_menu_is_open() && keel_nav_depth() == d0;
    pressed_for(KEEL_BUTTON_MENU, keel_motion.long_press + 150);
    bool long_np = keel_nav_showing(keel_scr_now_playing, NULL) && !keel_menu_is_open();
    pressed_for(KEEL_BUTTON_HOME, 60);
    int d3 = keel_nav_depth();
    bool ok = d0 == 3 && d1 == 2 && menu_short && menu_closed && long_np && d3 == 1;
    printf("%-24s depth %d, back %d | menu %s, back %s | long menu %s | home %d  %s\n", "buttons", d0, d1,
           menu_short ? "opens" : "NO", menu_closed ? "closes" : "NO", long_np ? "now playing" : "NO", d3,
           ok ? "ok" : "FAIL");
    return ok ? 0 : 1;
}

/* A long press on a song row opens its menu; "like" there likes it. */
static int check_item_menu(void)
{
    keel_demo_show(find_demo("06-album"));
    platform_pump(600);
    const keel_library_t * lib = keel_library();
    int32_t t = lib->albums[3].first_track;
    bool before = keel_library_track_liked(t);
    /* First track row: the album page puts it below the art and header. */
    lv_obj_t * row = NULL;
    lv_obj_t * col = lv_obj_get_child(lv_obj_get_child(lv_screen_active(), 0), 0);
    for(uint32_t i = 0; col && i < lv_obj_get_child_count(col) && !row; i++) {
        lv_obj_t * c = lv_obj_get_child(col, (int32_t)i);
        if(lv_obj_get_group(c)) row = c;
    }
    if(!row) {
        printf("%-24s no row found  FAIL\n", "item menu");
        return 1;
    }
    lv_obj_scroll_to_view(row, LV_ANIM_OFF);
    platform_pump(100);
    lv_area_t a;
    lv_obj_get_coords(row, &a);
    finger = (lv_point_t){(a.x1 + a.x2) / 2, (a.y1 + a.y2) / 2};
    finger_down = true;
    platform_pump(keel_motion.long_press + 200);
    finger_down = false;
    platform_pump(400);
    bool opened = keel_menu_is_open();
    bool tapped_through = keel_nav_showing(keel_scr_now_playing, NULL);
    /* Tap the sheet's third row: like (or unlike). */
    lv_obj_t * sheet = lv_obj_get_child(lv_layer_top(), -1);
    int32_t seen = 0;
    for(uint32_t i = 0; opened && i < lv_obj_get_child_count(sheet); i++) {
        lv_obj_t * c = lv_obj_get_child(sheet, (int32_t)i);
        if(lv_obj_get_group(c) && ++seen == 3) {
            lv_obj_send_event(c, LV_EVENT_SHORT_CLICKED, NULL);
            break;
        }
    }
    platform_pump(300);
    bool liked = keel_library_track_liked(t) != before && !keel_menu_is_open();
    keel_library_set_track_liked(t, before);
    bool ok = opened && !tapped_through && liked;
    printf("%-24s long press %s, tap %s, like %s  %s\n", "item menu", opened ? "opens menu" : "NO MENU",
           tapped_through ? "ALSO FIRED" : "not fired", liked ? "toggles" : "NO", ok ? "ok" : "FAIL");
    return ok ? 0 : 1;
}

/* Tap the n-th (1-based) row of the open menu sheet. */
static bool tap_menu_row(int32_t n)
{
    if(!keel_menu_is_open()) return false;
    lv_obj_t * sheet = lv_obj_get_child(lv_layer_top(), -1);
    int32_t seen = 0;
    for(uint32_t i = 0; i < lv_obj_get_child_count(sheet); i++) {
        lv_obj_t * c = lv_obj_get_child(sheet, (int32_t)i);
        if(lv_obj_get_group(c) && ++seen == n) {
            lv_obj_send_event(c, LV_EVENT_SHORT_CLICKED, NULL);
            platform_pump(300);
            return true;
        }
    }
    return false;
}

/* Delete through the real confirmation: a song that isn't playing (stay
 * put), then the one that is (the player moves on). Runs last: deletes
 * last for the session. */
static int check_delete(void)
{
    keel_demo_show(find_demo("04-music-songs"));
    platform_pump(600);
    int depth = keel_nav_depth();
    int32_t playing = keel_player_track();
    int32_t other = keel_library()->albums[5].first_track;
    keel_action_ask_delete_song(other);
    platform_pump(300);
    bool asked = keel_menu_is_open();
    tap_menu_row(1); /* "delete" */
    bool gone = keel_library_track_deleted(other) && !keel_menu_is_open() && keel_nav_depth() == depth;

    keel_action_ask_delete_song(playing);
    platform_pump(300);
    tap_menu_row(1);
    int32_t now = keel_player_track();
    bool moved = keel_library_track_deleted(playing) && now >= 0 && now != playing && !keel_library_track_deleted(now);

    bool ok = asked && gone && moved;
    printf("%-24s confirm %s, delete %s, playing one %s  %s\n", "delete", asked ? "asks" : "NO", gone ? "stays put" : "FAIL",
           moved ? "skips on" : "FAIL", ok ? "ok" : "FAIL");
    return ok ? 0 : 1;
}

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

    slow |= check_buttons();
    slow |= check_item_menu();
    slow |= check_delete();

    printf(slow ? "bench: FAIL\n" : "bench: all scenarios hold 60 fps\n");
    return slow;
}
