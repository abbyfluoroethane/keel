/* Now playing: art on top; under it the song name (the primary line, in
 * `heading`, up to two lines) and the artist; at the bottom the energy bar
 * (playback progress), times and three word controls.
 *
 * Body is 764 tall. From the top: art 480, overline 24, song 48 or 96,
 * artist 30. From the bottom: controls 72. The bar and times (9 + space-1
 * + 30) sit centered in the space between. Nothing scrolls.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "library.h"
#include "nav.h"
#include "player.h"
#include "screens.h"
#include "widgets.h"

#define CONTROLS 3
#define TARGETS (4 + CONTROLS)
#define DIM_OPA 128           /* art opa while idle, about 50% */
#define FLICK_MIN KS(40)      /* px and ... */
#define FLICK_MS 300          /* ... ms for a flick to skip */

typedef enum { SLIDE_OUT, SLIDE_IN, SLIDE_BACK } slide_t;

typedef struct {
    lv_obj_t * art_clip;
    lv_obj_t * art;
    lv_obj_t * overline;
    lv_obj_t * info;    /* column: song, artist */
    lv_obj_t * track;
    lv_obj_t * artist;
    lv_obj_t * bar;
    lv_obj_t * elapsed;
    lv_obj_t * total;
    lv_obj_t * ctl[CONTROLS];  /* previous, play or pause, next */
    lv_obj_t * play_label;
    lv_obj_t * targets[TARGETS];  /* everything that hides when idle */
    lv_timer_t * idle_timer;

    int32_t shown_album, shown_artist, shown_sec;
    char overline_text[32];

    /* Swipe */
    lv_point_t start;
    uint32_t press_tick;
    int32_t drag_dx;
    bool armed, dragging, busy;
    slide_t slide_kind;
    int dir;  /* +1 next, -1 previous, of the skip in flight */

    int32_t y_info, y_ctl;  /* top of the song block, top of the controls */

    /* Idle fade */
    int32_t fade_v;
    bool hidden;

    /* Anim handles: never dereferenced, only compared by the anim engine. */
    char slide_tag, fade_tag;
} np_t;

/* ---- Small helpers ---------------------------------------------------- */

static void label_set(lv_obj_t * l, const char * s)
{
    if(strcmp(lv_label_get_text(l), s) != 0) lv_label_set_text(l, s);
}

static void art_apply_dim(np_t * np)
{
    if(!np->art) return;
    lv_obj_set_style_opa(np->art, (lv_opa_t)(DIM_OPA + np->fade_v * (255 - DIM_OPA) / 255), 0);
}

/* ---- Idle: hide the large static text --------------------------------- */

static void fade_apply(np_t * np, int32_t v)
{
    np->fade_v = v;
    for(int i = 0; i < TARGETS; i++) lv_obj_set_style_opa(np->targets[i], (lv_opa_t)v, 0);
    art_apply_dim(np);
}

static void fade_exec(lv_anim_t * a, int32_t v)
{
    fade_apply(lv_anim_get_user_data(a), v);
}

static void set_hidden(np_t * np, bool hide)
{
    np->hidden = hide;
    lv_anim_delete(&np->fade_tag, NULL);
    int32_t to = hide ? 0 : 255;
    if(keel_reduced_motion()) {
        fade_apply(np, to);
        return;
    }
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, &np->fade_tag);
    lv_anim_set_user_data(&a, np);
    lv_anim_set_custom_exec_cb(&a, fade_exec);
    lv_anim_set_values(&a, np->fade_v, to);
    lv_anim_set_duration(&a, keel_motion.duration);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);
}

static void idle_cb(lv_timer_t * t)
{
    np_t * np = lv_timer_get_user_data(t);
    bool idle = lv_display_get_inactive_time(NULL) >= keel_motion.idle_hide;
    if(idle != np->hidden) set_hidden(np, idle);
}

/* ---- Skipping ---------------------------------------------------------- */

static void slide_exec(lv_anim_t * a, int32_t v)
{
    np_t * np = lv_anim_get_user_data(a);
    lv_obj_set_style_translate_x(np->art, v, 0);
}

static void slide(np_t * np, int32_t from, int32_t to, slide_t kind);

static void do_skip(np_t * np)
{
    if(np->dir > 0) keel_player_next();
    else keel_player_prev();
    /* The observer has put the new art in place. It enters from the side
     * the old one left toward. */
    if(keel_reduced_motion()) {
        lv_obj_set_style_translate_x(np->art, 0, 0);
        np->busy = false;
        return;
    }
    slide(np, np->dir * KEEL_HOR_RES, 0, SLIDE_IN);
}

static void slide_finished(np_t * np, slide_t kind)
{
    if(kind == SLIDE_OUT) do_skip(np);
    else np->busy = false;
}

static void slide_completed_cb(lv_anim_t * a)
{
    np_t * np = lv_anim_get_user_data(a);
    slide_finished(np, np->slide_kind);
}

static void slide(np_t * np, int32_t from, int32_t to, slide_t kind)
{
    np->slide_kind = kind;
    int32_t dist = LV_ABS(to - from);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, &np->slide_tag);
    lv_anim_set_user_data(&a, np);
    lv_anim_set_custom_exec_cb(&a, slide_exec);
    lv_anim_set_completed_cb(&a, slide_completed_cb);
    lv_anim_set_values(&a, from, to);
    lv_anim_set_duration(&a, LV_MAX(keel_motion.duration / 2, keel_motion.duration * (uint32_t)dist / KEEL_HOR_RES));
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);
}

/* dir +1 next, -1 previous; `from` is where the art is now. */
static void skip(np_t * np, int dir, int32_t from)
{
    if(np->busy || !keel_player_active()) return;
    np->dir = dir;
    if(keel_reduced_motion()) {
        do_skip(np);
        return;
    }
    np->busy = true;
    slide(np, from, -dir * KEEL_HOR_RES, SLIDE_OUT);
}

static void art_cb(lv_event_t * e)
{
    np_t * np = lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    lv_point_t p;
    lv_indev_get_point(lv_indev_active(), &p);

    if(code == LV_EVENT_PRESSED) {
        np->armed = !np->busy;
        np->dragging = false;
        np->start = p;
        np->drag_dx = 0;
        np->press_tick = lv_tick_get();
    }
    else if(code == LV_EVENT_PRESSING) {
        if(!np->armed) return;
        int32_t dx = p.x - np->start.x;
        int32_t dy = p.y - np->start.y;
        if(!np->dragging) {
            if(LV_ABS(dx) < KS(8) || LV_ABS(dx) < LV_ABS(dy)) return;
            np->dragging = true;
        }
        np->drag_dx = dx;
        lv_obj_set_style_translate_x(np->art, dx, 0);
    }
    else if(np->armed) {  /* RELEASED or PRESS_LOST */
        np->armed = false;
        if(!np->dragging) return;
        np->dragging = false;
        int32_t dx = np->drag_dx;
        bool flick = LV_ABS(dx) >= FLICK_MIN && lv_tick_elaps(np->press_tick) <= FLICK_MS;
        if(LV_ABS(dx) >= KEEL_HOR_RES / 4 || flick) {
            skip(np, dx < 0 ? 1 : -1, dx);
        }
        else if(keel_reduced_motion()) {
            lv_obj_set_style_translate_x(np->art, 0, 0);
        }
        else {
            np->busy = true;
            slide(np, dx, 0, SLIDE_BACK);
        }
    }
}

/* ---- Controls and keys ------------------------------------------------- */

static void control_cb(lv_event_t * e)
{
    np_t * np = lv_event_get_user_data(e);
    lv_obj_t * c = lv_event_get_target_obj(e);
    if(np->hidden) {
        /* The first touch only wakes the screen. */
        set_hidden(np, false);
        return;
    }
    if(c == np->ctl[0]) skip(np, -1, 0);
    else if(c == np->ctl[1]) keel_player_toggle();
    else skip(np, 1, 0);
}

static lv_obj_t * control_create(np_t * np, lv_obj_t * body, int idx, const char * text, lv_align_t align,
                                 int32_t x_off, int32_t y)
{
    int32_t w = KEEL_HOR_RES / CONTROLS;
    lv_obj_t * c = lv_obj_create(body);
    lv_obj_add_style(c, keel_style_plain(), 0);
    lv_obj_add_style(c, keel_style_row(), 0);
    lv_obj_add_style(c, keel_style_row_pressed(), LV_STATE_PRESSED);
    lv_obj_add_style(c, keel_style_focus_ring(), LV_STATE_FOCUS_KEY);
    lv_obj_set_scrollable(c, false);
    lv_obj_set_overflow_visible(c, true);
    lv_obj_set_pos(c, idx * w, y);
    lv_obj_set_size(c, w, keel_space.touch_min);
    lv_obj_add_event_cb(c, control_cb, LV_EVENT_CLICKED, np);
    keel_nav_focusable(c);

    lv_obj_t * l = keel_text(c, KEEL_TYPE_SUBHEADING, text);
    lv_obj_align(l, align, x_off, 0);
    if(idx == 1) np->play_label = l;
    return c;
}

/* ---- Observers ----------------------------------------------------------- */

/* The song name gets one line, or two when it needs them; past two it ends
 * in an ellipsis (LVGL only adds one when the height is fixed). */
/* Center the bar and times between the song block and the controls. */
static void place_progress(np_t * np, int32_t song_h)
{
    int32_t info_bottom = np->y_info + song_h + keel_type_line_height(KEEL_TYPE_SMALL);
    int32_t block_h = keel_space.bar_edge + keel_space.space_1 + keel_type_line_height(KEEL_TYPE_DATA);
    int32_t y_bar = info_bottom + (np->y_ctl - info_bottom - block_h) / 2;
    int32_t y_times = y_bar + keel_space.bar_edge + keel_space.space_1;
    lv_obj_set_y(np->bar, y_bar);
    lv_obj_set_y(np->elapsed, y_times);
    lv_obj_align(np->total, LV_ALIGN_TOP_RIGHT, -keel_space.margin, y_times);
}

static void set_track(np_t * np, const char * title)
{
    if(strcmp(lv_label_get_text(np->track), title) == 0) return;
    int32_t line = keel_type_line_height(KEEL_TYPE_HEADING);
    lv_point_t size;
    lv_text_get_size(&size, title, keel_type_font(KEEL_TYPE_HEADING),
                     lv_obj_get_style_text_letter_space(np->track, LV_PART_MAIN), 0,
                     KEEL_HOR_RES - 2 * keel_space.margin, LV_TEXT_FLAG_NONE);
    bool two = size.y > lv_font_get_line_height(keel_type_font(KEEL_TYPE_HEADING));
    lv_obj_set_height(np->track, two ? 2 * line : line);
    lv_label_set_text(np->track, title);
    place_progress(np, two ? 2 * line : line);
}

static void update_cb(lv_observer_t * obs, lv_subject_t * s)
{
    np_t * np = lv_observer_get_user_data(obs);
    const keel_library_t * lib = keel_library();
    int32_t t = keel_player_track();
    if(t < 0) {
        set_track(np, "nothing playing");
        return;
    }
    const keel_track_t * tr = &lib->tracks[t];
    const keel_album_t * al = &lib->albums[tr->album];

    if(tr->album != np->shown_album) {
        np->shown_album = tr->album;
        if(np->art) lv_obj_delete(np->art);
        np->art = keel_cover(np->art_clip, tr->album, KEEL_ART_FULL);
        art_apply_dim(np);
    }
    if(al->artist != np->shown_artist) {
        np->shown_artist = al->artist;
        lv_label_set_text(np->artist, lib->artists[al->artist].name);
    }
    int32_t qpos = keel_player_queue_pos();
    char buf[64];
    snprintf(buf, sizeof(buf), "Track %d of %d", (int)qpos + 1, (int)keel_player_queue_len());
    if(strcmp(buf, np->overline_text) != 0) {
        strcpy(np->overline_text, buf);
        keel_text_set(np->overline, buf);
    }
    set_track(np, tr->title);
    keel_format_time(buf, sizeof(buf), tr->duration);
    label_set(np->total, buf);
    label_set(np->play_label, keel_player_playing() ? "pause" : "play");

    /* Times change once a second; touch the label only then. */
    int32_t pos_ms = (int32_t)keel_player_position_ms();
    int32_t sec = pos_ms / 1000;
    if(sec != np->shown_sec) {
        np->shown_sec = sec;
        keel_format_time(buf, sizeof(buf), sec);
        label_set(np->elapsed, buf);
    }
    /* permille = pos_ms * 1000 / (duration_s * 1000) */
    keel_energy_bar_set(np->bar, tr->duration > 0 ? pos_ms / tr->duration : 0);
}

static void delete_cb(lv_event_t * e)
{
    np_t * np = lv_event_get_user_data(e);
    lv_anim_delete(&np->slide_tag, NULL);
    lv_anim_delete(&np->fade_tag, NULL);
    lv_timer_delete(np->idle_timer);
    free(np);
}

/* ---- Screen -------------------------------------------------------------- */

void keel_scr_now_playing(lv_obj_t * body, void * arg)
{
    np_t * np = calloc(1, sizeof(*np));
    np->shown_album = np->shown_artist = np->shown_sec = -1;
    np->fade_v = 255;
    lv_obj_add_event_cb(body, delete_cb, LV_EVENT_DELETE, np);

    int32_t m = keel_space.margin;
    int32_t y_over = KEEL_ART_FULL + keel_space.space_1;
    int32_t y_info = y_over + keel_type_line_height(KEEL_TYPE_OVERLINE);
    /* The bottom block is anchored to the bottom edge. */
    int32_t y_ctl = keel_nav_body_height() - keel_space.touch_min;
    np->y_info = y_info;
    np->y_ctl = y_ctl;

    /* Art: the swipe surface. The child moves, the clip stays. */
    np->art_clip = keel_box(body);
    lv_obj_set_size(np->art_clip, KEEL_ART_FULL, KEEL_ART_FULL);
    lv_obj_set_clickable(np->art_clip, true);
    lv_event_code_t swipe[] = {LV_EVENT_PRESSED, LV_EVENT_PRESSING, LV_EVENT_RELEASED, LV_EVENT_PRESS_LOST};
    for(size_t i = 0; i < sizeof(swipe) / sizeof(swipe[0]); i++) lv_obj_add_event_cb(np->art_clip, art_cb, swipe[i], np);

    np->overline = keel_text(body, KEEL_TYPE_OVERLINE, "");
    lv_obj_set_pos(np->overline, m, y_over);

    /* The song is the primary line; the artist sits under it. */
    np->info = keel_box(body);
    lv_obj_set_pos(np->info, m, y_info);
    lv_obj_set_width(np->info, KEEL_HOR_RES - 2 * m);
    lv_obj_set_flex_flow(np->info, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_overflow_visible(np->info, true);

    np->track = keel_text(np->info, KEEL_TYPE_HEADING, "");
    lv_obj_set_size(np->track, LV_PCT(100), keel_type_line_height(KEEL_TYPE_HEADING));
    lv_label_set_long_mode(np->track, LV_LABEL_LONG_MODE_DOTS);

    np->artist = keel_text(np->info, KEEL_TYPE_SMALL, "");
    lv_obj_set_size(np->artist, LV_PCT(100), keel_type_line_height(KEEL_TYPE_SMALL));
    lv_label_set_long_mode(np->artist, LV_LABEL_LONG_MODE_DOTS);
    lv_obj_set_style_text_color(np->artist, keel_color.muted, 0);

    np->bar = keel_energy_bar(body, KEEL_HOR_RES - 2 * m);
    lv_obj_set_x(np->bar, m);

    np->elapsed = keel_text(body, KEEL_TYPE_DATA, "0:00");
    lv_obj_set_style_text_color(np->elapsed, keel_color.muted, 0);
    lv_obj_set_x(np->elapsed, m);
    np->total = keel_text(body, KEEL_TYPE_DATA, "0:00");
    lv_obj_set_style_text_color(np->total, keel_color.muted, 0);
    place_progress(np, keel_type_line_height(KEEL_TYPE_HEADING));

    np->ctl[0] = control_create(np, body, 0, "previous", LV_ALIGN_LEFT_MID, m, y_ctl);
    np->ctl[1] = control_create(np, body, 1, "pause", LV_ALIGN_CENTER, 0, y_ctl);
    np->ctl[2] = control_create(np, body, 2, "next", LV_ALIGN_RIGHT_MID, -m, y_ctl);

    lv_obj_t * t[TARGETS] = {np->overline, np->info,   np->elapsed, np->total,
                             np->ctl[0],   np->ctl[1], np->ctl[2]};
    memcpy(np->targets, t, sizeof(t));

    np->idle_timer = lv_timer_create(idle_cb, 500, np);
    lv_group_focus_obj(np->ctl[1]);

    lv_subject_add_observer_obj(keel_player_subject(), update_cb, body, np);
    lv_subject_add_observer_obj(keel_player_pos_subject(), update_cb, body, np);
}
