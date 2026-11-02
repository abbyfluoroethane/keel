/* Keel building blocks. Screens are made from these and keel_text().
 * Nothing here draws a box, card, divider, shadow or rounded corner.
 */
#ifndef KEEL_WIDGETS_H
#define KEEL_WIDGETS_H

#include "lvgl.h"
#include "theme.h"

/* A plain container (transparent, no padding). Not clickable, not
 * scrollable. Use it for layout. */
lv_obj_t * keel_box(lv_obj_t * parent);

/* Overline above a title or hero: the signature pair. Left edge at the
 * page margin. `title_type` is KEEL_TYPE_TITLE or KEEL_TYPE_HERO (or
 * KEEL_TYPE_HEADING where a compact name fits better, e.g. the album page).
 * Returns the column; its children are [overline, title].
 *
 * Keel departs from design rule 4 here, by decision: an app's top-level
 * screen passes NULL and gets no overline; its title is enough. Pages
 * nested inside an app (artist, album, a settings page) keep it. */
lv_obj_t * keel_header(lv_obj_t * parent, const char * overline, const char * title, keel_type_t title_type);

/* A full-width list row: no box, no divider, hover fill on tap.
 *   primary    subheading, text color, one line, ellipsis when long
 *   secondary  small, muted, below primary (NULL for none)
 *   data       data style, muted, right-aligned (NULL for none), e.g. "3:41"
 * At least the minimum touch target tall. Text starts at the page margin.
 * `cb` gets LV_EVENT_SHORT_CLICKED (a tap, not a long press) with `user`
 * as user data. Keyboard-focusable. */
lv_obj_t * keel_row(lv_obj_t * parent, const char * primary, const char * secondary, const char * data,
                    lv_event_cb_t cb, void * user);

/* A row whose only text is one `title`-style word, e.g. the home menu. */
lv_obj_t * keel_title_row(lv_obj_t * parent, const char * text, lv_event_cb_t cb, void * user);

/* Square cover art at one of the KEEL_ART_* sizes. Falls back to a plain
 * `raised` square when the art has not been fetched. */
lv_obj_t * keel_cover(lv_obj_t * parent, int32_t album, int32_t size);

/* The energy bar: bar-start to accent, left to right, bar-edge tall.
 * The only gradient in Keel. Nothing may be drawn on or behind it.
 * `permille` 0..1000. */
lv_obj_t * keel_energy_bar(lv_obj_t * parent, int32_t width);
void keel_energy_bar_set(lv_obj_t * bar, int32_t permille);

/* A square status indicator with its text label next to it (rule 5).
 * `ok` picks status-ok or status-error. */
lv_obj_t * keel_status(lv_obj_t * parent, bool ok, const char * text);

/* Mini player bar on `raised`, aligned to the bottom of `parent`: a `line`
 * divider on top, one touch target of art, title and play/pause, then the
 * energy bar (playback progress) full width at the bottom. Hidden while
 * nothing plays. Tap opens now playing. */
lv_obj_t * keel_mini_player(lv_obj_t * parent);
/* Its total height, for laying out what sits above it. */
int32_t keel_mini_player_height(void);

/* ---- Pivot ------------------------------------------------------------
 * A horizontal strip of panes under a strip of `hero` headers. The
 * active header is `text`; the others are `dim`. The header strip moves
 * slower than the panes (parallax). Swipe or click a header to move;
 * left and right are Vela's back and menu buttons, not pane keys.
 * Panes scroll vertically and are flex columns.
 */
/* `overline` may be NULL (see keel_header). */
lv_obj_t * keel_pivot(lv_obj_t * parent, const char * overline, const char * const * names, int32_t count,
                      int32_t initial);
lv_obj_t * keel_pivot_pane(lv_obj_t * pivot, int32_t index);
int32_t keel_pivot_active(lv_obj_t * pivot);
void keel_pivot_go(lv_obj_t * pivot, int32_t index, bool anim);
/* Fires LV_EVENT_VALUE_CHANGED on the pivot when the active pane changes. */

#endif /* KEEL_WIDGETS_H */
