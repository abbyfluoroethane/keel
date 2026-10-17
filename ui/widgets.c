#include "widgets.h"

#include <stdlib.h>
#include <string.h>

#include "library.h"
#include "nav.h"
#include "player.h"
#include "screens.h"

lv_obj_t * keel_box(lv_obj_t * parent)
{
    lv_obj_t * o = lv_obj_create(parent);
    lv_obj_add_style(o, keel_style_plain(), 0);
    lv_obj_set_scrollable(o, false);
    lv_obj_set_clickable(o, false);
    lv_obj_set_size(o, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    return o;
}

lv_obj_t * keel_header(lv_obj_t * parent, const char * overline, const char * title, keel_type_t title_type)
{
    lv_obj_t * col = keel_box(parent);
    lv_obj_set_width(col, LV_PCT(100));
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_left(col, keel_space.margin, 0);
    /* Air between the status bar and the overline. */
    lv_obj_set_style_pad_top(col, keel_space.space_2, 0);
    /* A hero may run off the right edge; a title wraps inside the margin. */
    lv_obj_set_style_pad_right(col, title_type == KEEL_TYPE_HERO ? 0 : keel_space.margin, 0);
    lv_obj_set_overflow_visible(col, true);

    if(overline) keel_text(col, KEEL_TYPE_OVERLINE, overline);
    lv_obj_t * t = keel_text(col, title_type, title);
    if(title_type == KEEL_TYPE_HERO) {
        lv_label_set_long_mode(t, LV_LABEL_LONG_MODE_CLIP);
        lv_obj_set_width(t, LV_SIZE_CONTENT);
    }
    else {
        lv_obj_set_width(t, LV_PCT(100));
    }
    return col;
}

/* ---- Rows ------------------------------------------------------------- */

static lv_obj_t * row_base(lv_obj_t * parent, lv_event_cb_t cb, void * user)
{
    lv_obj_t * row = lv_obj_create(parent);
    lv_obj_add_style(row, keel_style_plain(), 0);
    lv_obj_add_style(row, keel_style_row(), 0);
    lv_obj_add_style(row, keel_style_row_pressed(), LV_STATE_PRESSED);
    lv_obj_add_style(row, keel_style_focus_ring(), LV_STATE_FOCUS_KEY);
    lv_obj_set_scrollable(row, false);
    lv_obj_set_overflow_visible(row, true);
    /* Let a drag on a row scroll its list instead of being eaten. */
    lv_obj_set_scroll_chain(row, true);
    lv_obj_set_width(row, LV_PCT(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_set_style_min_height(row, keel_space.touch_min, 0);
    lv_obj_set_style_pad_hor(row, keel_space.margin, 0);
    if(cb) lv_obj_add_event_cb(row, cb, LV_EVENT_CLICKED, user);
    keel_nav_focusable(row);
    return row;
}

lv_obj_t * keel_row(lv_obj_t * parent, const char * primary, const char * secondary, const char * data,
                    lv_event_cb_t cb, void * user)
{
    lv_obj_t * row = row_base(parent, cb, user);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row, keel_space.space_4, 0);
    /* Center one line in the touch target; two lines fill it. */
    int32_t text_h = keel_type_line_height(KEEL_TYPE_SUBHEADING) +
                     (secondary ? keel_type_line_height(KEEL_TYPE_SMALL) : 0);
    int32_t pad = LV_MAX(0, (keel_space.touch_min - text_h) / 2);
    lv_obj_set_style_pad_ver(row, pad, 0);

    lv_obj_t * col = keel_box(row);
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_grow(col, 1);
    lv_obj_set_overflow_visible(col, true);
    lv_obj_set_event_bubble(col, true);

    /* One line each: LVGL only adds the ellipsis when the height is fixed. */
    lv_obj_t * p = keel_text(col, KEEL_TYPE_SUBHEADING, primary);
    lv_obj_set_size(p, LV_PCT(100), keel_type_line_height(KEEL_TYPE_SUBHEADING));
    lv_label_set_long_mode(p, LV_LABEL_LONG_MODE_DOTS);
    if(secondary) {
        lv_obj_t * s = keel_text(col, KEEL_TYPE_SMALL, secondary);
        lv_obj_set_size(s, LV_PCT(100), keel_type_line_height(KEEL_TYPE_SMALL));
        lv_label_set_long_mode(s, LV_LABEL_LONG_MODE_DOTS);
        lv_obj_set_style_text_color(s, keel_color.muted, 0);
    }
    if(data) {
        lv_obj_t * d = keel_text(row, KEEL_TYPE_DATA, data);
        lv_obj_set_style_text_color(d, keel_color.muted, 0);
    }
    return row;
}

lv_obj_t * keel_title_row(lv_obj_t * parent, const char * text, lv_event_cb_t cb, void * user)
{
    lv_obj_t * row = row_base(parent, cb, user);
    lv_obj_t * t = keel_text(row, KEEL_TYPE_TITLE, text);
    lv_label_set_long_mode(t, LV_LABEL_LONG_MODE_CLIP);
    return row;
}

/* ---- Art, bar, status ------------------------------------------------- */

lv_obj_t * keel_cover(lv_obj_t * parent, int32_t album, int32_t size)
{
    const char * src = keel_library_cover(album, size);
    lv_obj_t * o;
    if(src) {
        o = lv_image_create(parent);
        lv_image_set_src(o, src);
    }
    else {
        o = keel_box(parent);
        lv_obj_set_style_bg_color(o, keel_color.raised, 0);
        lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    }
    lv_obj_set_size(o, size, size);
    lv_obj_set_clickable(o, false);
    return o;
}

lv_obj_t * keel_energy_bar(lv_obj_t * parent, int32_t width)
{
    lv_obj_t * bar = keel_box(parent);
    lv_obj_set_size(bar, width, keel_space.bar_edge);

    lv_obj_t * fill = keel_box(bar);
    lv_obj_set_size(fill, 0, keel_space.bar_edge);
    lv_obj_set_style_bg_opa(fill, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(fill, keel_color.bar_start, 0);
    lv_obj_set_style_bg_grad_color(fill, keel_color.accent, 0);
    lv_obj_set_style_bg_grad_dir(fill, LV_GRAD_DIR_HOR, 0);
    return bar;
}

void keel_energy_bar_set(lv_obj_t * bar, int32_t permille)
{
    lv_obj_t * fill = lv_obj_get_child(bar, 0);
    int32_t w = lv_obj_get_width(bar) * LV_CLAMP(0, permille, 1000) / 1000;
    if(lv_obj_get_width(fill) != w) lv_obj_set_width(fill, w);
}

lv_obj_t * keel_status(lv_obj_t * parent, bool ok, const char * text)
{
    lv_obj_t * row = keel_box(parent);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row, keel_space.space_2, 0);

    lv_obj_t * sq = keel_box(row);
    lv_obj_set_size(sq, keel_space.space_2, keel_space.space_2);
    lv_obj_set_style_bg_color(sq, ok ? keel_color.status_ok : keel_color.status_error, 0);
    lv_obj_set_style_bg_opa(sq, LV_OPA_COVER, 0);

    keel_text(row, KEEL_TYPE_SMALL, text);
    return row;
}

/* ---- Mini player ------------------------------------------------------ */

typedef struct {
    lv_obj_t * bar;
    lv_obj_t * art_holder;
    lv_obj_t * title;
    lv_obj_t * artist;
    lv_obj_t * toggle;
    int32_t shown_track;
} mini_t;

static void mini_update(lv_observer_t * obs, lv_subject_t * s)
{
    mini_t * m = lv_observer_get_user_data(obs);
    int32_t t = keel_player_track();
    if(t < 0) {
        lv_obj_set_hidden(m->bar, true);
        return;
    }
    lv_obj_set_hidden(m->bar, false);
    const keel_library_t * lib = keel_library();
    const keel_track_t * tr = &lib->tracks[t];
    if(t != m->shown_track) {
        m->shown_track = t;
        lv_obj_clean(m->art_holder);
        keel_cover(m->art_holder, tr->album, KEEL_ART_THUMB);
        lv_label_set_text(m->title, tr->title);
        lv_label_set_text(m->artist, lib->artists[lib->albums[tr->album].artist].name);
    }
    lv_label_set_text(m->toggle, keel_player_playing() ? "pause" : "play");
}

static void mini_open_cb(lv_event_t * e)
{
    keel_nav_push(keel_scr_now_playing, NULL);
}

static void mini_toggle_cb(lv_event_t * e)
{
    keel_player_toggle();
}

static void mini_delete_cb(lv_event_t * e)
{
    free(lv_event_get_user_data(e));
}

lv_obj_t * keel_mini_player(lv_obj_t * parent)
{
    mini_t * m = calloc(1, sizeof(*m));
    m->shown_track = -1;

    lv_obj_t * bar = lv_obj_create(parent);
    m->bar = bar;
    lv_obj_add_style(bar, keel_style_plain(), 0);
    lv_obj_add_style(bar, keel_style_focus_ring(), LV_STATE_FOCUS_KEY);
    lv_obj_set_style_bg_color(bar, keel_color.raised, 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(bar, keel_color.hover, LV_STATE_PRESSED);
    lv_obj_set_scrollable(bar, false);
    lv_obj_set_size(bar, LV_PCT(100), keel_space.touch_min);
    lv_obj_align(bar, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(bar, keel_space.space_3, 0);
    lv_obj_add_event_cb(bar, mini_open_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(bar, mini_delete_cb, LV_EVENT_DELETE, m);
    keel_nav_focusable(bar);

    m->art_holder = keel_box(bar);
    lv_obj_set_size(m->art_holder, KEEL_ART_THUMB, KEEL_ART_THUMB);

    lv_obj_t * col = keel_box(bar);
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_grow(col, 1);
    lv_obj_set_event_bubble(col, true);
    m->title = keel_text(col, KEEL_TYPE_SUBHEADING, "");
    lv_obj_set_size(m->title, LV_PCT(100), keel_type_line_height(KEEL_TYPE_SUBHEADING));
    lv_label_set_long_mode(m->title, LV_LABEL_LONG_MODE_DOTS);
    m->artist = keel_text(col, KEEL_TYPE_SMALL, "");
    lv_obj_set_size(m->artist, LV_PCT(100), keel_type_line_height(KEEL_TYPE_SMALL));
    lv_label_set_long_mode(m->artist, LV_LABEL_LONG_MODE_DOTS);
    /* `muted` is for text on base only, so the artist on raised uses text. */

    m->toggle = keel_text(bar, KEEL_TYPE_LABEL, "pause");
    lv_obj_set_clickable(m->toggle, true);
    lv_obj_set_style_pad_hor(m->toggle, keel_space.margin, 0);
    lv_obj_set_ext_click_area(m->toggle, keel_space.touch_min / 2);
    lv_obj_add_event_cb(m->toggle, mini_toggle_cb, LV_EVENT_CLICKED, NULL);

    lv_subject_add_observer_obj(keel_player_subject(), mini_update, bar, m);
    return bar;
}

/* ---- Pivot ------------------------------------------------------------ */

typedef struct {
    lv_obj_t * root;
    lv_obj_t * headers;   /* strip of hero labels */
    lv_obj_t * strip;     /* horizontally scrolling panes */
    int32_t count;
    int32_t active;
    int32_t * hx;         /* x of each header inside the header strip */
    int32_t drag_from;    /* active pane when the finger went down */
    bool settling;        /* our own ease-out to a pane is running */
} pivot_t;

/* Headers left of the active one would show their tails at the left edge
 * (the strip sits at margin - hx). Fade them while the panes are at rest;
 * text_opa, not hidden, so the flex positions in hx stay valid. */
static void pivot_show_left_headers(pivot_t * p, bool show)
{
    for(int32_t i = 0; i < p->count; i++) {
        lv_obj_t * h = lv_obj_get_child(p->headers, i);
        lv_obj_set_style_text_opa(h, show || i >= p->active ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
    }
}

static void pivot_set_active(pivot_t * p, int32_t idx)
{
    if(idx == p->active) return;
    for(int32_t i = 0; i < p->count; i++) {
        lv_obj_t * h = lv_obj_get_child(p->headers, i);
        lv_obj_set_style_text_color(h, i == idx ? keel_color.text : keel_color.dim, 0);
    }
    p->active = idx;
    int32_t * st = keel_nav_state();
    if(st) *st = idx;
    lv_obj_send_event(p->root, LV_EVENT_VALUE_CHANGED, NULL);
}

/* Header follows the content at a slower rate: between pane i and i+1 it
 * travels from hx[i] to hx[i+1] while the content travels a full pane. */
static void pivot_place_headers(pivot_t * p, int32_t scroll_x)
{
    int32_t w = KEEL_HOR_RES;
    int32_t i = LV_CLAMP(0, scroll_x / w, p->count - 1);
    int32_t frac = scroll_x - i * w;
    int32_t x;
    if(i >= p->count - 1) x = p->hx[p->count - 1];
    else x = p->hx[i] + (p->hx[i + 1] - p->hx[i]) * frac / w;
    lv_obj_set_x(p->headers, keel_space.margin - x);
}

static void pivot_scroll_cb(lv_event_t * e)
{
    pivot_t * p = lv_event_get_user_data(e);
    int32_t sx = lv_obj_get_scroll_x(p->strip);
    int32_t idx = LV_CLAMP(0, (sx + KEEL_HOR_RES / 2) / KEEL_HOR_RES, p->count - 1);
    if(!keel_reduced_motion()) pivot_place_headers(p, sx);
    pivot_set_active(p, idx);
}

/* Settling to a pane is ours, not LVGL's snap: LVGL snaps only inside its
 * momentum phase and can skip panes. One pane per swipe, ease-out. */
static void settle_exec_cb(void * var, int32_t v)
{
    pivot_t * p = var;
    lv_obj_scroll_to_x(p->strip, v, LV_ANIM_OFF);
}

static void settle_done_cb(lv_anim_t * a)
{
    pivot_t * p = a->var;
    p->settling = false;
    pivot_place_headers(p, p->active * KEEL_HOR_RES);
    pivot_show_left_headers(p, false);
}

static void pivot_settle(pivot_t * p, int32_t index, bool anim)
{
    int32_t from = lv_obj_get_scroll_x(p->strip);
    int32_t to = index * KEEL_HOR_RES;
    lv_anim_delete(p, settle_exec_cb);
    if(!anim || keel_reduced_motion() || from == to) {
        /* settling stays true across the jump: it fires scroll begin/end,
         * which must not be read as a swipe. */
        p->settling = true;
        lv_obj_scroll_to_x(p->strip, to, LV_ANIM_OFF);
        pivot_set_active(p, index);
        p->settling = false;
        pivot_place_headers(p, to);
        pivot_show_left_headers(p, false);
        return;
    }
    p->settling = true;
    pivot_show_left_headers(p, true);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, p);
    lv_anim_set_values(&a, from, to);
    lv_anim_set_duration(&a, keel_motion.duration);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_set_exec_cb(&a, settle_exec_cb);
    lv_anim_set_completed_cb(&a, settle_done_cb);
    lv_anim_start(&a);
}

static void pivot_scroll_begin_cb(lv_event_t * e)
{
    pivot_t * p = lv_event_get_user_data(e);
    if(p->settling) return;
    p->drag_from = p->active;
    pivot_show_left_headers(p, true);
}

/* The finger is up: go one pane in the swipe direction if it travelled a
 * fifth of the width, otherwise back. */
static void pivot_scroll_end_cb(lv_event_t * e)
{
    pivot_t * p = lv_event_get_user_data(e);
    if(p->settling) return;
    int32_t dx = lv_obj_get_scroll_x(p->strip) - p->drag_from * KEEL_HOR_RES;
    int32_t to = p->drag_from;
    if(dx > KEEL_HOR_RES / 5) to++;
    else if(dx < -KEEL_HOR_RES / 5) to--;
    pivot_settle(p, LV_CLAMP(0, to, p->count - 1), true);
}

static void pivot_header_click_cb(lv_event_t * e)
{
    pivot_t * p = lv_event_get_user_data(e);
    lv_obj_t * h = lv_event_get_target_obj(e);
    keel_pivot_go(p->root, (int32_t)lv_obj_get_index(h), true);
}

static void pivot_delete_cb(lv_event_t * e)
{
    pivot_t * p = lv_event_get_user_data(e);
    lv_anim_delete(p, settle_exec_cb);
    free(p->hx);
    free(p);
}

lv_obj_t * keel_pivot(lv_obj_t * parent, const char * overline, const char * const * names, int32_t count,
                      int32_t initial)
{
    pivot_t * p = calloc(1, sizeof(*p));
    p->count = count;
    p->active = -1;
    p->hx = calloc((size_t)count, sizeof(int32_t));

    lv_obj_t * root = keel_box(parent);
    p->root = root;
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_user_data(root, p);
    lv_obj_add_event_cb(root, pivot_delete_cb, LV_EVENT_DELETE, p);

    if(overline) {
        lv_obj_t * ov = keel_text(root, KEEL_TYPE_OVERLINE, overline);
        lv_obj_set_style_margin_left(ov, keel_space.margin, 0);
        lv_obj_set_style_margin_top(ov, keel_space.space_2, 0);
    }

    /* Header row: a clip window the width of the screen, with the strip of
     * hero labels inside it moved by x. */
    lv_obj_t * win = keel_box(root);
    /* Content height: the hero's box can be taller than its spec line
     * (theme.c never trims into glyphs). */
    lv_obj_set_size(win, LV_PCT(100), LV_SIZE_CONTENT);
    if(!overline) lv_obj_set_style_margin_top(win, keel_space.space_2, 0);
    lv_obj_set_overflow_visible(win, true);

    p->headers = keel_box(win);
    lv_obj_set_flex_flow(p->headers, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(p->headers, keel_space.space_4, 0);
    lv_obj_set_overflow_visible(p->headers, true);
    for(int32_t i = 0; i < count; i++) {
        lv_obj_t * h = keel_text(p->headers, KEEL_TYPE_HERO, names[i]);
        lv_obj_set_style_text_color(h, keel_color.dim, 0);
        lv_obj_set_clickable(h, true);
        lv_obj_add_event_cb(h, pivot_header_click_cb, LV_EVENT_CLICKED, p);
    }

    p->strip = lv_obj_create(root);
    lv_obj_add_style(p->strip, keel_style_plain(), 0);
    lv_obj_set_width(p->strip, LV_PCT(100));
    lv_obj_set_flex_grow(p->strip, 1);
    lv_obj_set_flex_flow(p->strip, LV_FLEX_FLOW_ROW);
    lv_obj_set_scroll_dir(p->strip, LV_DIR_HOR);
    lv_obj_set_scrollbar_mode(p->strip, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_elastic(p->strip, false);
    lv_obj_set_scroll_momentum(p->strip, false);
    lv_obj_add_event_cb(p->strip, pivot_scroll_cb, LV_EVENT_SCROLL, p);
    lv_obj_add_event_cb(p->strip, pivot_scroll_begin_cb, LV_EVENT_SCROLL_BEGIN, p);
    lv_obj_add_event_cb(p->strip, pivot_scroll_end_cb, LV_EVENT_SCROLL_END, p);

    for(int32_t i = 0; i < count; i++) {
        lv_obj_t * pane = lv_obj_create(p->strip);
        lv_obj_add_style(pane, keel_style_plain(), 0);
        lv_obj_set_size(pane, KEEL_HOR_RES, LV_PCT(100));
        lv_obj_set_flex_flow(pane, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_scroll_dir(pane, LV_DIR_VER);
        lv_obj_set_scrollbar_mode(pane, LV_SCROLLBAR_MODE_OFF);
        /* Clickable (with no handler) so a drag on empty space below the
         * last row still hits the pane and scrolls. */
        lv_obj_set_scroll_chain_hor(pane, true);
        lv_obj_set_scroll_chain_ver(pane, false);
    }

    lv_obj_update_layout(root);
    int32_t x = 0;
    for(int32_t i = 0; i < count; i++) {
        p->hx[i] = x;
        x += lv_obj_get_width(lv_obj_get_child(p->headers, i)) + keel_space.space_4;
    }

    keel_pivot_go(root, LV_CLAMP(0, initial, count - 1), false);
    return root;
}

lv_obj_t * keel_pivot_pane(lv_obj_t * pivot, int32_t index)
{
    pivot_t * p = lv_obj_get_user_data(pivot);
    return lv_obj_get_child(p->strip, index);
}

int32_t keel_pivot_active(lv_obj_t * pivot)
{
    pivot_t * p = lv_obj_get_user_data(pivot);
    return p->active;
}

void keel_pivot_go(lv_obj_t * pivot, int32_t index, bool anim)
{
    pivot_t * p = lv_obj_get_user_data(pivot);
    pivot_settle(p, LV_CLAMP(0, index, p->count - 1), anim);
}
