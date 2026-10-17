/* Screen builders. Each fills a body made by keel_nav (see nav.h). */
#ifndef KEEL_SCREENS_H
#define KEEL_SCREENS_H

#include "lvgl.h"

void keel_scr_home(lv_obj_t * body, void * arg);
/* arg: initial pane as (void *)(intptr_t)n: 0 artists, 1 albums, 2 songs, 3 playlists */
void keel_scr_music(lv_obj_t * body, void * arg);
/* arg: (void *)(intptr_t)album index */
void keel_scr_album(lv_obj_t * body, void * arg);
/* arg: (void *)(intptr_t)artist index */
void keel_scr_artist(lv_obj_t * body, void * arg);
/* arg: (void *)(intptr_t)playlist index */
void keel_scr_playlist(lv_obj_t * body, void * arg);
void keel_scr_now_playing(lv_obj_t * body, void * arg);
void keel_scr_radio(lv_obj_t * body, void * arg);
void keel_scr_settings(lv_obj_t * body, void * arg);
/* arg: (void *)(intptr_t)page: 0 display, 1 sound, 2 wireless, 3 about */
void keel_scr_settings_page(lv_obj_t * body, void * arg);
/* Podcasts is on the home menu but out of scope for the demo. */
void keel_scr_podcasts(lv_obj_t * body, void * arg);

#endif /* KEEL_SCREENS_H */
