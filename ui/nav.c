#include "nav.h"

#include <stdio.h>
#include <time.h>

#include "keel.h"
#include "menu.h"
#include "player.h"
#include "screens.h"
#include "theme.h"
#include "widgets.h"

#define MAX_DEPTH 16

static void attach_group(lv_group_t * g);

static const keel_screen_build_t home_build = keel_scr_home;

typedef struct {
    keel_screen_build_t build;
    void * arg;
    int32_t state;
} entry_t;

static entry_t stack[MAX_DEPTH];
static int depth;
static lv_group_t * group;
static bool reduced;
static keel_key_hook_t key_hook;
static void * key_hook_user;
static keel_menu_cb_t menu_cb;
static void * menu_user;

static lv_obj_t * status_bar;
static lv_obj_t * status_wifi;
static lv_obj_t * status_bt;
static lv_obj_t * status_clock;
static lv_obj_t * status_battery;
static uint32_t burn_step;

static lv_timer_t * menu_hold;  /* running while the menu button is held */
static bool menu_long_done;

bool keel_reduced_motion(void) { return reduced; }
int keel_nav_depth(void) { return depth; }
int32_t keel_nav_body_height(void) { return KEEL_VER_RES - keel_space.status_bar_h; }

int32_t * keel_nav_state(void)
{
    return depth > 0 ? &stack[depth - 1].state : NULL;
}

/* ---- Status bar --------------------------------------------------------
 * On the top layer, so it persists across screens. Text in `data`, color
 * `muted` (burn-in rule). The whole bar shifts by 1px every few minutes.
 */

static void status_tick(lv_timer_t * t)
{
    time_t now = time(NULL);
    struct tm tm;
    localtime_r(&now, &tm);
    char buf[16];
    snprintf(buf, sizeof(buf), "%d:%02d", tm.tm_hour, tm.tm_min);
    lv_label_set_text(status_clock, buf);
}

static void burn_tick(lv_timer_t * t)
{
    /* Walk a 2x2 square: (0,0) (1,0) (1,1) (0,1). */
    static const int8_t dx[4] = {0, 1, 1, 0};
    static const int8_t dy[4] = {0, 0, 1, 1};
    burn_step = (burn_step + 1) % 4;
    lv_obj_set_pos(status_bar, dx[burn_step], dy[burn_step]);
}

static void status_bar_create(void)
{
    status_bar = lv_obj_create(lv_layer_top());
    lv_obj_add_style(status_bar, keel_style_plain(), 0);
    lv_obj_set_size(status_bar, KEEL_HOR_RES, keel_space.status_bar_h);
    lv_obj_set_scrollable(status_bar, false);
    lv_obj_set_clickable(status_bar, false);

    /* Radio indicators, left. Phosphor icons: a word would not fit, and
     * phones have taught these two shapes well. */
    lv_obj_t * radios = keel_box(status_bar);
    lv_obj_set_flex_flow(radios, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(radios, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(radios, keel_space.space_2, 0);
    lv_obj_align(radios, LV_ALIGN_LEFT_MID, keel_space.margin, 0);
    status_wifi = keel_icon(radios, KEEL_ICON_WIFI_HIGH);
    status_bt = keel_icon(radios, KEEL_ICON_BLUETOOTH);
    lv_obj_set_style_text_color(status_wifi, keel_color.muted, 0);
    lv_obj_set_style_text_color(status_bt, keel_color.muted, 0);
    keel_status_set_radios(3, 1);

    status_clock = keel_text(status_bar, KEEL_TYPE_DATA, "0:00");
    lv_obj_set_style_text_color(status_clock, keel_color.muted, 0);
    lv_obj_align(status_clock, LV_ALIGN_CENTER, 0, 0);

    /* No battery driver yet; a fixed value stands in. */
    status_battery = keel_text(status_bar, KEEL_TYPE_DATA, "82%");
    lv_obj_set_style_text_color(status_battery, keel_color.muted, 0);
    lv_obj_align(status_battery, LV_ALIGN_RIGHT_MID, -keel_space.margin, 0);

    status_tick(NULL);
    lv_timer_create(status_tick, 1000, NULL);
    lv_timer_create(burn_tick, keel_motion.burn_shift, NULL);
}

void keel_status_set_radios(int32_t wifi_bars, int32_t bluetooth)
{
    static const char * const wifi[] = {KEEL_ICON_WIFI_LOW, KEEL_ICON_WIFI_LOW, KEEL_ICON_WIFI_MEDIUM,
                                        KEEL_ICON_WIFI_HIGH};
    lv_obj_set_hidden(status_wifi, wifi_bars < 0);
    if(wifi_bars >= 0) lv_label_set_text(status_wifi, wifi[LV_MIN(wifi_bars, 3)]);
    lv_obj_set_hidden(status_bt, bluetooth <= 0);
    if(bluetooth > 0) lv_label_set_text(status_bt, bluetooth > 1 ? KEEL_ICON_BLUETOOTH_CONNECTED : KEEL_ICON_BLUETOOTH);
}

/* ---- Keys ------------------------------------------------------------- */

/* Keys other than Vela's buttons: Up moves focus back; Tab, Shift+Tab and
 * Enter are handled by LVGL's group itself. */
static void key_cb(lv_event_t * e)
{
    uint32_t key = lv_event_get_key(e);
    if(key_hook && key_hook(key, key_hook_user)) return;
    if(key == LV_KEY_UP) lv_group_focus_prev(lv_obj_get_group(lv_event_get_target_obj(e)));
}

void keel_nav_focusable(lv_obj_t * obj)
{
    lv_group_add_obj(lv_group_get_default(), obj);
    lv_obj_set_scroll_on_focus(obj, true);
    lv_obj_add_event_cb(obj, key_cb, LV_EVENT_KEY, NULL);
}

/* ---- Buttons ------------------------------------------------------------ */

static void menu_hold_cb(lv_timer_t * t)
{
    menu_hold = NULL;
    menu_long_done = true;
    keel_menu_close();
    keel_nav_now_playing();
}

void keel_button_event(keel_button_t button, bool pressed)
{
    switch(button) {
        case KEEL_BUTTON_BACK:
            if(!pressed) return;
            if(keel_menu_is_open()) keel_menu_close();
            else keel_nav_back();
            return;
        case KEEL_BUTTON_HOME:
            if(!pressed) return;
            keel_menu_close();
            keel_nav_home();
            return;
        case KEEL_BUTTON_MENU:
            /* Short or long is only known at release or after the hold time. */
            if(pressed) {
                menu_long_done = false;
                if(menu_hold) lv_timer_delete(menu_hold);
                menu_hold = lv_timer_create(menu_hold_cb, keel_motion.long_press, NULL);
                lv_timer_set_repeat_count(menu_hold, 1);
                lv_timer_set_auto_delete(menu_hold, true);
                return;
            }
            if(menu_hold) {
                lv_timer_delete(menu_hold);
                menu_hold = NULL;
            }
            if(menu_long_done) return;
            if(keel_menu_is_open()) keel_menu_close();
            else if(menu_cb) menu_cb(menu_user);
            return;
    }
}

bool keel_nav_showing(keel_screen_build_t build, void * arg)
{
    return depth > 0 && stack[depth - 1].build == build && stack[depth - 1].arg == arg;
}

void keel_nav_now_playing(void)
{
    if(!keel_player_active()) return;
    if(depth > 0 && stack[depth - 1].build == keel_scr_now_playing) return;
    keel_nav_push(keel_scr_now_playing, NULL);
}

void keel_nav_set_menu(keel_menu_cb_t cb, void * user)
{
    menu_cb = cb;
    menu_user = user;
}

void keel_nav_set_key_hook(keel_key_hook_t hook, void * user)
{
    key_hook = hook;
    key_hook_user = user;
}

void keel_nav_use_group(lv_group_t * g)
{
    attach_group(g ? g : group);
}

static void attach_group(lv_group_t * g)
{
    lv_group_set_default(g);
    for(lv_indev_t * i = lv_indev_get_next(NULL); i; i = lv_indev_get_next(i))
        if(lv_indev_get_type(i) == LV_INDEV_TYPE_KEYPAD) lv_indev_set_group(i, g);
}

/* ---- Stagger ---------------------------------------------------------- */

static void anim_translate_x(void * obj, int32_t v)
{
    lv_obj_set_style_translate_x(obj, v, 0);
}

static void anim_opa(void * obj, int32_t v)
{
    lv_obj_set_style_opa(obj, (lv_opa_t)v, 0);
}

void keel_nav_stagger(lv_obj_t * obj, int32_t index)
{
    if(reduced) return;
    uint32_t delay = (uint32_t)LV_MIN(index, (int32_t)keel_motion.stagger_max) * keel_motion.stagger_step;

    lv_obj_set_style_translate_x(obj, keel_motion.stagger_dist, 0);
    lv_obj_set_style_opa(obj, LV_OPA_TRANSP, 0);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_duration(&a, keel_motion.duration);
    lv_anim_set_delay(&a, delay);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_set_values(&a, keel_motion.stagger_dist, 0);
    lv_anim_set_exec_cb(&a, anim_translate_x);
    lv_anim_start(&a);

    lv_anim_set_values(&a, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_exec_cb(&a, anim_opa);
    lv_anim_start(&a);
}

/* ---- Stack ------------------------------------------------------------ */

static void delete_group_cb(void * g)
{
    lv_group_delete(g);
}

static void show_top(void)
{
    entry_t * en = &stack[depth - 1];
    lv_obj_t * old = lv_screen_active();

    /* A menu belongs to the screen it opened on. */
    keel_menu_close();
    lv_group_t * old_group = group;

    key_hook = NULL;
    key_hook_user = NULL;
    menu_cb = NULL;
    menu_user = NULL;
    group = lv_group_create();
    lv_group_set_wrap(group, false);
    attach_group(group);

    lv_obj_t * scr = lv_obj_create(NULL);
    lv_obj_add_style(scr, keel_style_screen(), 0);
    lv_obj_set_scrollable(scr, false);

    lv_obj_t * body = lv_obj_create(scr);
    lv_obj_add_style(body, keel_style_plain(), 0);
    lv_obj_set_pos(body, 0, keel_space.status_bar_h);
    lv_obj_set_size(body, KEEL_HOR_RES, keel_nav_body_height());
    lv_obj_set_scrollable(body, false);
    lv_obj_set_clickable(body, false);

    en->build(body, en->arg);

    lv_screen_load(scr);
    /* The old screen may be running the event that got us here (a row's
     * click), so free it and its group after the event returns. */
    if(old && old != scr) lv_obj_delete_async(old);
    if(old_group) lv_async_call(delete_group_cb, old_group);
}

void keel_nav_push(keel_screen_build_t build, void * arg)
{
    if(depth >= MAX_DEPTH) return;
    stack[depth++] = (entry_t){build, arg, -1};
    show_top();
}

void keel_nav_back(void)
{
    if(depth <= 1) return;
    depth--;
    show_top();
}

void keel_nav_home(void)
{
    if(depth == 1 && stack[0].build == home_build) return;
    keel_nav_reset(home_build, NULL);
}

void keel_nav_reset(keel_screen_build_t build, void * arg)
{
    depth = 0;
    keel_nav_push(build, arg);
}

void keel_nav_init(bool reduced_motion)
{
    reduced = reduced_motion;
    status_bar_create();
}
