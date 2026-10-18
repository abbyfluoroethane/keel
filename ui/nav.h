/* Screen stack, keyboard focus, and the status bar.
 *
 * A screen is a build function. Pushing one creates a fresh LVGL screen
 * with a black body below the status bar, calls the builder to fill the
 * body, then deletes the previous screen. Going back rebuilds the previous
 * entry (screens are cheap; RAM on the device is not infinite).
 */
#ifndef KEEL_NAV_H
#define KEEL_NAV_H

#include "lvgl.h"

/* Fill `body` (480 x 800 - status bar, black, no padding, not scrollable). */
typedef void (*keel_screen_build_t)(lv_obj_t * body, void * arg);

void keel_nav_init(bool reduced_motion);

void keel_nav_push(keel_screen_build_t build, void * arg);
void keel_nav_back(void);
/* Clear the stack and show `build` as the only screen. */
void keel_nav_reset(keel_screen_build_t build, void * arg);
int keel_nav_depth(void);

/* One int that survives going back to this entry, e.g. the pivot pane.
 * Starts at -1 on push, so a builder can tell first open from a rebuild.
 * Valid during and after the builder of the current entry. */
int32_t * keel_nav_state(void);

/* Make `obj` reachable by keyboard: Up and Shift+Tab move focus back, Tab
 * moves it forward, Enter clicks, a long Enter is a long press. Also
 * scrolls it into view when focused. It joins the current focus group:
 * the screen's, or an open menu's. (Vela's hardware buttons are not keys;
 * see keel_button_event in keel.h.) */
void keel_nav_focusable(lv_obj_t * obj);

/* The home button: back to the home screen from anywhere. */
void keel_nav_home(void);

/* A long press of the menu button: open now playing, unless it is already
 * showing or nothing is loaded. */
void keel_nav_now_playing(void);

/* True when the top screen is `build` opened with `arg`. */
bool keel_nav_showing(keel_screen_build_t build, void * arg);

/* What a short press of the menu button does on this screen: usually opens
 * a context menu (keel_menu_open). Without one the press does nothing.
 * Cleared when the screen changes. */
typedef void (*keel_menu_cb_t)(void * user);
void keel_nav_set_menu(keel_menu_cb_t cb, void * user);

/* Send keyboard focus to `g` (a menu's group), or back to the screen's
 * group with NULL. */
void keel_nav_use_group(lv_group_t * g);

/* A screen can see the other keys first. Return true to consume the key.
 * Cleared when the screen changes. */
typedef bool (*keel_key_hook_t)(uint32_t key, void * user);
void keel_nav_set_key_hook(keel_key_hook_t hook, void * user);

/* Enter animation for list rows: slide in from the right with a short
 * stagger by `index`. Does nothing under --reduced-motion. Call after the
 * row is created; safe to call for every row (late rows enter together). */
void keel_nav_stagger(lv_obj_t * obj, int32_t index);

bool keel_reduced_motion(void);

/* Radio state for the status bar indicators. A backend reports it; the
 * laptop demo shows full Wi-Fi and Bluetooth on.
 *   wifi_bars  -1 off, 0..3 signal (0 shows as low)
 *   bluetooth  0 off, 1 on, 2 connected to a device
 * An indicator whose radio is off is hidden, not dimmed: dim must never be
 * the only copy of information. */
void keel_status_set_radios(int32_t wifi_bars, int32_t bluetooth);

/* Body height below the status bar. */
int32_t keel_nav_body_height(void);

#endif /* KEEL_NAV_H */
