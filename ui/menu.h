/* Context menus: a sheet of word rows over the current screen.
 *
 * The menu button opens a screen's menu (keel_nav_set_menu); a long press
 * on an item (a song, an album) opens that item's menu. A menu is type on
 * black like everything else: an overline and a title, then one row per
 * action, then "close". Back or the menu button closes it, home closes it
 * and goes home, and changing screens closes it.
 */
#ifndef KEEL_MENU_H
#define KEEL_MENU_H

#include "lvgl.h"

typedef struct {
    const char * label;        /* lowercase word(s): "play next", "delete" */
    void (*cb)(void * user);   /* runs after the menu has closed */
    void * user;
} keel_menu_item_t;

#define KEEL_MENU_MAX_ITEMS 10

/* Open a menu. `overline` is sentence-case source text (shown uppercase);
 * `title` is shown as written. Items are copied. */
void keel_menu_open(const char * overline, const char * title, const keel_menu_item_t * items, int32_t count);

/* Ask before a destructive action. `note` (may be NULL) is one muted line
 * under the question. Rows: `yes` and "cancel". */
void keel_menu_confirm(const char * overline, const char * question, const char * note, const char * yes,
                       void (*cb)(void * user), void * user);

void keel_menu_close(void);
bool keel_menu_is_open(void);

/* Long-pressing `obj` (touch, or holding Enter while it has focus) calls
 * `open(obj, user)`, which opens that item's menu. Pair it with
 * LV_EVENT_SHORT_CLICKED for the tap action: LVGL still sends
 * LV_EVENT_CLICKED after a long press. */
typedef void (*keel_item_menu_cb_t)(lv_obj_t * obj, void * user);
void keel_menu_on_long_press(lv_obj_t * obj, keel_item_menu_cb_t open, void * user);

#endif /* KEEL_MENU_H */
