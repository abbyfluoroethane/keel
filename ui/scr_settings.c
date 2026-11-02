#include <stdint.h>

#include "nav.h"
#include "screens.h"
#include "widgets.h"

static const char * const page_names[] = {"display", "sound", "wireless", "about"};

static void page_cb(lv_event_t * e)
{
    keel_nav_push(keel_scr_settings_page, lv_event_get_user_data(e));
}

void keel_scr_settings(lv_obj_t * body, void * arg)
{
    lv_obj_set_flex_flow(body, LV_FLEX_FLOW_COLUMN);
    keel_header(body, NULL, "settings", KEEL_TYPE_TITLE);

    for(int32_t i = 0; i < 4; i++) {
        lv_obj_t * r = keel_row(body, page_names[i], NULL, NULL, page_cb, (void *)(intptr_t)i);
        keel_nav_stagger(r, i);
    }
}

void keel_scr_settings_page(lv_obj_t * body, void * arg)
{
    int32_t page = LV_CLAMP(0, (int32_t)(intptr_t)arg, 3);
    lv_obj_set_flex_flow(body, LV_FLEX_FLOW_COLUMN);
    keel_header(body, "Settings", page_names[page], KEEL_TYPE_TITLE);

    /* Scroll container so a longer page never runs under the screen edge. */
    lv_obj_t * list = lv_obj_create(body);
    lv_obj_add_style(list, keel_style_plain(), 0);
    lv_obj_set_width(list, LV_PCT(100));
    lv_obj_set_flex_grow(list, 1);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scroll_dir(list, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t * rows[4] = {0};
    int32_t n = 0;
    switch(page) {
        case 0:
            rows[n++] = keel_row(list, "brightness", NULL, "80%", NULL, NULL);
            rows[n++] = keel_row(list, "screen timeout", NULL, "30 s", NULL, NULL);
            rows[n++] = keel_row(list, "reduced motion", NULL, keel_reduced_motion() ? "on" : "off", NULL, NULL);
            break;
        case 1:
            rows[n++] = keel_row(list, "volume", NULL, "60%", NULL, NULL);
            rows[n++] = keel_row(list, "output", "headphones", NULL, NULL, NULL);
            break;
        case 2: {
            lv_obj_t * st = keel_status(list, true, "connected");
            lv_obj_set_style_pad_left(st, keel_space.margin, 0);
            lv_obj_set_style_min_height(st, keel_space.touch_min, 0);
            rows[n++] = keel_row(list, "network", "vela-lab", NULL, NULL, NULL);
            rows[n++] = keel_row(list, "signal", NULL, "\xE2\x88\x92" "52 dBm", NULL, NULL);
            break;
        }
        default:
            rows[n++] = keel_row(list, "version", NULL, "keel " KEEL_VERSION, NULL, NULL);
            rows[n++] = keel_row(list, "ui", NULL, "lvgl 9.6", NULL, NULL);
            rows[n++] = keel_row(list, "license", "MIT", NULL, NULL, NULL);
            break;
    }
    for(int32_t i = 0; i < n; i++) keel_nav_stagger(rows[i], i);
}
