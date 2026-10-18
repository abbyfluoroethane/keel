#include <stdint.h>

#include "actions.h"
#include "nav.h"
#include "screens.h"
#include "widgets.h"

static void go_cb(lv_event_t * e)
{
    keel_screen_build_t build = (keel_screen_build_t)lv_event_get_user_data(e);
    keel_nav_push(build, NULL);
}

static void go_music_cb(lv_event_t * e)
{
    keel_nav_push(keel_scr_music, (void *)(intptr_t)0);
}

static void menu_cb(void * user)
{
    keel_menu_library("home");
}

static void back_cb(lv_event_t * e)
{
    keel_nav_back();
}

void keel_scr_home(lv_obj_t * body, void * arg)
{
    /* The list area ends above the mini player, which sits at the bottom. */
    lv_obj_t * list = keel_box(body);
    lv_obj_set_size(list, LV_PCT(100), keel_nav_body_height() - keel_space.touch_min);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_top(list, keel_space.space_4, 0);

    /* No overline on an app's top level; see keel_header. */
    lv_obj_t * rows[4];
    rows[0] = keel_title_row(list, "music", go_music_cb, NULL);
    rows[1] = keel_title_row(list, "radio", go_cb, (void *)keel_scr_radio);
    rows[2] = keel_title_row(list, "podcasts", go_cb, (void *)keel_scr_podcasts);
    rows[3] = keel_title_row(list, "settings", go_cb, (void *)keel_scr_settings);
    for(int32_t i = 0; i < 4; i++) keel_nav_stagger(rows[i], i);

    keel_mini_player(body);
    keel_nav_set_menu(menu_cb, NULL);
}

void keel_scr_podcasts(lv_obj_t * body, void * arg)
{
    lv_obj_set_flex_flow(body, LV_FLEX_FLOW_COLUMN);
    keel_header(body, NULL, "podcasts", KEEL_TYPE_TITLE);

    lv_obj_t * t = keel_text(body, KEEL_TYPE_BODY, "podcasts arrive in a later build.");
    lv_obj_set_style_text_color(t, keel_color.muted, 0);
    lv_obj_set_width(t, LV_PCT(100));
    lv_obj_set_style_pad_hor(t, keel_space.margin, 0);
    lv_obj_set_style_pad_bottom(t, keel_space.space_4, 0);

    keel_row(body, "back to home", NULL, NULL, back_cb, NULL);
}
