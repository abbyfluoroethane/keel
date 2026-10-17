#include <stdio.h>
#include <stdlib.h>

#include "nav.h"
#include "screens.h"
#include "widgets.h"

/* FM band in tenths of a MHz. The strip is a ruler 12 px per 0.1 MHz;
 * `pos` is the ruler offset in px, 0 at the bottom of the band. */
#define F_MIN 875
#define F_MAX 1080
#define F_START 1013
#define STEP 12
#define POS_MAX ((F_MAX - F_MIN) * STEP)

#define STRIP_H 96
#define BASE_Y 56   /* tick baseline, below the marker's top */
#define LABEL_Y 62

typedef struct radio_s radio_t;
typedef struct {
    radio_t * r;
    int32_t freq;
    lv_obj_t * marker;
} preset_t;

struct radio_s {
    lv_obj_t * strip;
    lv_obj_t * numeral;
    int32_t pos;
    int32_t freq;   /* tenths of MHz shown in the numeral */
    int32_t goal;   /* where keys and eases are heading */
    preset_t presets[4];
};

static const struct {
    const char * name;
    const char * text;
    int32_t freq;
} preset_defs[4] = {
    {"community radio", "88.7", 887},
    {"late night jazz", "91.5", 915},
    {"college radio", "101.3", 1013},
    {"news", "104.1", 1041},
};

static int32_t freq_of_pos(int32_t pos)
{
    return F_MIN + (pos + STEP / 2) / STEP;
}

/* Redraw only what changed: the strip always, the numeral and the
 * selected preset only when the snapped frequency changes. */
static void set_pos(radio_t * r, int32_t pos)
{
    pos = LV_CLAMP(0, pos, POS_MAX);
    if(pos == r->pos) return;
    r->pos = pos;
    lv_obj_invalidate(r->strip);

    int32_t f = freq_of_pos(pos);
    if(f == r->freq) return;
    r->freq = f;
    char buf[8];
    snprintf(buf, sizeof(buf), "%d.%d", f / 10, f % 10);
    keel_text_set(r->numeral, buf);
    for(int32_t i = 0; i < 4; i++) lv_obj_set_hidden(r->presets[i].marker, r->presets[i].freq != f);
}

static void pos_anim_cb(void * var, int32_t v)
{
    set_pos(var, v);
}

static void tune_to(radio_t * r, int32_t freq, bool anim)
{
    freq = LV_CLAMP(F_MIN, freq, F_MAX);
    r->goal = freq;
    int32_t target = (freq - F_MIN) * STEP;
    lv_anim_delete(r, pos_anim_cb);
    if(!anim || keel_reduced_motion() || target == r->pos) {
        set_pos(r, target);
        return;
    }
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, r);
    lv_anim_set_exec_cb(&a, pos_anim_cb);
    lv_anim_set_values(&a, r->pos, target);
    lv_anim_set_duration(&a, keel_motion.duration);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);
}

/* ---- Strip ------------------------------------------------------------ */

static void tick(lv_layer_t * layer, lv_draw_fill_dsc_t * d, int32_t x, int32_t y2, int32_t h, int32_t w)
{
    lv_area_t a = {x - w / 2, y2 - h, x - w / 2 + w - 1, y2 - 1};
    lv_draw_fill(layer, d, &a);
}

static void strip_draw_cb(lv_event_t * e)
{
    radio_t * r = lv_event_get_user_data(e);
    lv_layer_t * layer = lv_event_get_layer(e);
    lv_area_t box;
    lv_obj_get_coords(r->strip, &box);
    int32_t w = lv_area_get_width(&box);
    int32_t cx = box.x1 + w / 2;
    int32_t base = box.y1 + BASE_Y;

    lv_draw_fill_dsc_t fill;
    lv_draw_fill_dsc_init(&fill);

    lv_draw_label_dsc_t lbl;
    lv_draw_label_dsc_init(&lbl);
    lbl.font = keel_type_font(KEEL_TYPE_DATA);
    lbl.color = keel_color.muted;
    lbl.align = LV_TEXT_ALIGN_CENTER;
    lbl.text_local = 1;

    /* Only the ticks inside the strip, plus room for a label's half width. */
    int32_t k0 = LV_MAX(0, (r->pos - w / 2 - 36) / STEP);
    int32_t k1 = LV_MIN(F_MAX - F_MIN, (r->pos + w / 2 + 36) / STEP + 1);
    for(int32_t k = k0; k <= k1; k++) {
        int32_t x = cx + k * STEP - r->pos;
        int32_t f = F_MIN + k;
        if(f % 10 == 0) {
            fill.color = keel_color.muted;
            tick(layer, &fill, x, base, KS(18), 3);
            char buf[8];
            snprintf(buf, sizeof(buf), "%d", f / 10);
            lbl.text = buf;
            lv_area_t a = {x - 36, box.y1 + LABEL_Y, x + 36, box.y1 + LABEL_Y + keel_type_line_height(KEEL_TYPE_DATA)};
            lv_draw_label(layer, &lbl, &a);
        }
        else if(f % 10 == 5) {
            fill.color = keel_color.dim;
            tick(layer, &fill, x, base, KS(13), 3);
        }
        else {
            fill.color = keel_color.dim;
            tick(layer, &fill, x, base, KS(8), 2);
        }
    }

    /* The selected marker: decoration only, the numeral states the value. */
    fill.color = keel_color.accent;
    tick(layer, &fill, cx, base, BASE_Y, 4);
}

static void strip_press_cb(lv_event_t * e)
{
    radio_t * r = lv_event_get_user_data(e);
    lv_anim_delete(r, pos_anim_cb);
}

static void strip_pressing_cb(lv_event_t * e)
{
    radio_t * r = lv_event_get_user_data(e);
    lv_indev_t * indev = lv_indev_active();
    if(!indev) return;
    lv_point_t v;
    lv_indev_get_vect(indev, &v);
    if(v.x) set_pos(r, r->pos - v.x);
}

static void strip_release_cb(lv_event_t * e)
{
    radio_t * r = lv_event_get_user_data(e);
    tune_to(r, freq_of_pos(r->pos), true);
}

static void strip_delete_cb(lv_event_t * e)
{
    radio_t * r = lv_event_get_user_data(e);
    lv_anim_delete(r, pos_anim_cb);
    free(r);
}

static void preset_cb(lv_event_t * e)
{
    preset_t * p = lv_event_get_user_data(e);
    tune_to(p->r, p->freq, true);
}

/* ---- Screen ----------------------------------------------------------- */

void keel_scr_radio(lv_obj_t * body, void * arg)
{
    radio_t * r = calloc(1, sizeof(*r));
    r->pos = -1;
    r->freq = -1;

    lv_obj_set_flex_flow(body, LV_FLEX_FLOW_COLUMN);
    keel_header(body, NULL, "radio", KEEL_TYPE_TITLE);

    /* The one loud element, with its unit beneath. */
    lv_obj_t * col = keel_box(body);
    lv_obj_set_width(col, LV_PCT(100));
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_left(col, keel_space.margin, 0);
    r->numeral = keel_text(col, KEEL_TYPE_NUMERAL, "00.0");
    lv_obj_t * unit = keel_text(col, KEEL_TYPE_DATA, "MHz · fm · stereo");
    lv_obj_set_style_text_color(unit, keel_color.muted, 0);

    /* The tuning strip: one object, ticks drawn on demand. */
    lv_obj_t * strip = lv_obj_create(body);
    r->strip = strip;
    lv_obj_add_style(strip, keel_style_plain(), 0);
    lv_obj_add_style(strip, keel_style_row(), 0);
    lv_obj_set_scrollable(strip, false);
    lv_obj_set_size(strip, LV_PCT(100), STRIP_H);
    lv_obj_set_style_margin_top(strip, keel_space.space_3, 0);
    lv_obj_add_event_cb(strip, strip_draw_cb, LV_EVENT_DRAW_MAIN, r);
    lv_obj_add_event_cb(strip, strip_press_cb, LV_EVENT_PRESSED, r);
    lv_obj_add_event_cb(strip, strip_pressing_cb, LV_EVENT_PRESSING, r);
    lv_obj_add_event_cb(strip, strip_release_cb, LV_EVENT_RELEASED, r);
    lv_obj_add_event_cb(strip, strip_release_cb, LV_EVENT_PRESS_LOST, r);
    lv_obj_add_event_cb(strip, strip_delete_cb, LV_EVENT_DELETE, r);
    /* Touch only: the arrow keys are Vela's buttons, and the presets
     * below give keyboard users the stations. */

    /* Presets scroll if they ever outgrow the body. */
    lv_obj_t * list = lv_obj_create(body);
    lv_obj_add_style(list, keel_style_plain(), 0);
    lv_obj_set_width(list, LV_PCT(100));
    lv_obj_set_flex_grow(list, 1);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scroll_dir(list, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_margin_top(list, keel_space.space_3, 0);
    lv_obj_set_clickable(list, false);

    for(int32_t i = 0; i < 4; i++) {
        preset_t * p = &r->presets[i];
        p->r = r;
        p->freq = preset_defs[i].freq;
        lv_obj_t * row = keel_row(list, preset_defs[i].name, NULL, preset_defs[i].text, preset_cb, p);
        /* Selected marker in the left margin; radius 0 comes from keel_box. */
        p->marker = keel_box(row);
        lv_obj_set_floating(p->marker, true);
        lv_obj_set_size(p->marker, keel_space.space_2, keel_space.space_2);
        lv_obj_set_style_bg_color(p->marker, keel_color.accent, 0);
        lv_obj_set_style_bg_opa(p->marker, LV_OPA_COVER, 0);
        lv_obj_align(p->marker, LV_ALIGN_LEFT_MID, -(keel_space.margin - keel_space.space_2), 0);
        keel_nav_stagger(row, i);
    }

    tune_to(r, F_START, false);
}
