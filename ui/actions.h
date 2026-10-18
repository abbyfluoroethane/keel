/* Things you can do to music, shared by every screen's menus.
 *
 * Item menus open on a long press (see keel_menu_on_long_press). Pass the
 * long-pressed object as `origin`: deleting the item hides it in place, so
 * the list keeps its scroll position. `origin` may be NULL.
 */
#ifndef KEEL_ACTIONS_H
#define KEEL_ACTIONS_H

#include "lvgl.h"

/* Item menus. */
void keel_menu_song(int32_t track, lv_obj_t * origin);
void keel_menu_album(int32_t album, lv_obj_t * origin);
void keel_menu_artist(int32_t artist, lv_obj_t * origin);

/* keel_menu_on_long_press adapters: `user` is the index as (void *)(intptr_t). */
void keel_long_press_song(lv_obj_t * obj, void * user);
void keel_long_press_album(lv_obj_t * obj, void * user);
void keel_long_press_artist(lv_obj_t * obj, void * user);

/* Screen menus that more than one screen shows. */
void keel_menu_album_page(int32_t album);  /* menu button on an album page */
void keel_menu_now_playing(void);          /* menu button on now playing */
void keel_menu_library(const char * title); /* home and the music pivot */

/* Actions. Each one that starts playback also opens now playing. */
void keel_action_play_tracks(const int32_t * tracks, int32_t count, int32_t index);
void keel_action_shuffle_tracks(const int32_t * tracks, int32_t count);
void keel_action_play_album(int32_t album, bool shuffle);
void keel_action_play_artist(int32_t artist, bool shuffle);
void keel_action_shuffle_all(void);
void keel_action_go_album(int32_t album);
void keel_action_go_artist(int32_t artist);
/* Ask, then delete. The menu item "delete" calls these. */
void keel_action_ask_delete_song(int32_t track);
void keel_action_ask_delete_album(int32_t album);

/* Every track not deleted, in `tracks_by_title` order. Returns the count;
 * the caller frees *out. */
int32_t keel_visible_tracks(int32_t ** out);

#endif /* KEEL_ACTIONS_H */
