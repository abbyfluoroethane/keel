#include "menu.h"

#include <stdlib.h>
#include <string.h>

#include "nav.h"
#include "theme.h"
#include "widgets.h"

static lv_obj_t * sheet;
static lv_group_t * sheet_group;
static keel_menu_item_t items[KEEL_MENU_MAX_ITEMS];
static int32_t item_count;

static void delete_group_cb(void * g)
{
    lv_group_delete(g);
}

bool keel_menu_is_open(void)
{
    return sheet != NULL;
}

void keel_menu_close(void)
{
    if(!sheet) return;
    /* The sheet may be running the event that closes it (a row's click),
     * so hide it now and free it after the event returns. */
    lv_obj_set_hidden(sheet, true);
    lv_obj_delete_async(sheet);
    sheet = NULL;
    keel_nav_use_group(NULL);
    lv_async_call(delete_group_cb, sheet_group);
    sheet_group = NULL;
}

static void row_cb(lv_event_t * e)
{
    int32_t i = (int32_t)(intptr_t)lv_event_get_user_data(e);
    keel_menu_item_t item = i >= 0 ? items[i] : (keel_menu_item_t){0};
    keel_menu_close();
    if(item.cb) item.cb(item.user);
}

static void anim_translate_y(void * obj, int32_t v)
{
    lv_obj_set_style_translate_y(obj, v, 0);
}

static void anim_opa(void * obj, int32_t v)
{
    lv_obj_set_style_opa(obj, (lv_opa_t)v, 0);
}

static void open_sheet(const char * overline, const char * title, const char * note, const keel_menu_item_t * list,
                       int32_t count, const char * dismiss)
{
    keel_menu_close();
    item_count = LV_MIN(count, KEEL_MENU_MAX_ITEMS);
    memcpy(items, list, sizeof(keel_menu_item_t) * (size_t)item_count);

    sheet_group = lv_group_create();
    lv_group_set_wrap(sheet_group, false);
    keel_nav_use_group(sheet_group);

    /* Under the status bar, over the whole body. Black and clickable, so
     * touches never reach the screen beneath. */
    sheet = lv_obj_create(lv_layer_top());
    lv_obj_add_style(sheet, keel_style_screen(), 0);
    lv_obj_set_pos(sheet, 0, keel_space.status_bar_h);
    lv_obj_set_size(sheet, KEEL_HOR_RES, keel_nav_body_height());
    lv_obj_set_flex_flow(sheet, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scroll_dir(sheet, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(sheet, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_bottom(sheet, keel_space.space_8, 0);

    lv_obj_t * head = keel_header(sheet, overline, title, KEEL_TYPE_HEADING);
    lv_obj_set_style_margin_bottom(head, note ? 0 : keel_space.space_3, 0);
    if(note) {
        lv_obj_t * n = keel_text(sheet, KEEL_TYPE_SMALL, note);
        lv_obj_set_width(n, LV_PCT(100));
        lv_obj_set_style_pad_hor(n, keel_space.margin, 0);
        lv_obj_set_style_text_color(n, keel_color.muted, 0);
        lv_obj_set_style_margin_bottom(n, keel_space.space_3, 0);
    }

    for(int32_t i = 0; i <= item_count; i++) {
        bool last = i == item_count;
        lv_obj_t * row = keel_row(sheet, last ? dismiss : items[i].label, NULL, NULL, row_cb,
                                  (void *)(intptr_t)(last ? -1 : i));
        keel_nav_stagger(row, i);
    }
    lv_group_focus_obj(lv_obj_get_child(sheet, note ? 2 : 1));

    if(keel_reduced_motion()) return;
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, sheet);
    lv_anim_set_duration(&a, keel_motion.duration);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_set_values(&a, keel_motion.stagger_dist, 0);
    lv_anim_set_exec_cb(&a, anim_translate_y);
    lv_anim_start(&a);
    lv_anim_set_values(&a, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_exec_cb(&a, anim_opa);
    lv_anim_start(&a);
}

void keel_menu_open(const char * overline, const char * title, const keel_menu_item_t * list, int32_t count)
{
    open_sheet(overline, title, NULL, list, count, "close");
}

void keel_menu_confirm(const char * overline, const char * question, const char * note, const char * yes,
                       void (*cb)(void * user), void * user)
{
    keel_menu_item_t item = {yes, cb, user};
    open_sheet(overline, question, note, &item, 1, "cancel");
}

/* ---- Long press -------------------------------------------------------- */

typedef struct {
    keel_item_menu_cb_t open;
    void * user;
} long_press_t;

static void long_press_cb(lv_event_t * e)
{
    long_press_t * lp = lv_event_get_user_data(e);
    lp->open(lv_event_get_target_obj(e), lp->user);
}

static void long_press_free_cb(lv_event_t * e)
{
    free(lv_event_get_user_data(e));
}

void keel_menu_on_long_press(lv_obj_t * obj, keel_item_menu_cb_t open, void * user)
{
    long_press_t * lp = malloc(sizeof(*lp));
    lp->open = open;
    lp->user = user;
    lv_obj_add_event_cb(obj, long_press_cb, LV_EVENT_LONG_PRESSED, lp);
    lv_obj_add_event_cb(obj, long_press_free_cb, LV_EVENT_DELETE, lp);
}
