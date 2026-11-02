#include "actions.h"

#include <stdio.h>
#include <stdlib.h>

#include "library.h"
#include "menu.h"
#include "nav.h"
#include "player.h"
#include "screens.h"

#define IDX(p) ((int32_t)(intptr_t)(p))
#define PTR(i) ((void *)(intptr_t)(i))

/* The object an item menu was opened from, hidden when its item is
 * deleted. It clears itself if the object is deleted first (the menu was
 * closed and the screen changed), so it is never a dangling pointer. */
static lv_obj_t * origin_obj;

static void origin_deleted_cb(lv_event_t * e)
{
    origin_obj = NULL;
}

static void set_origin(lv_obj_t * obj)
{
    if(origin_obj) lv_obj_remove_event_cb(origin_obj, origin_deleted_cb);
    origin_obj = obj;
    if(obj) lv_obj_add_event_cb(obj, origin_deleted_cb, LV_EVENT_DELETE, NULL);
}

static void hide_origin(void)
{
    if(origin_obj) lv_obj_set_hidden(origin_obj, true);
    set_origin(NULL);
}

/* ---- Playing ----------------------------------------------------------- */

void keel_action_play_tracks(const int32_t * tracks, int32_t count, int32_t index)
{
    if(count <= 0) return;
    keel_player_play(tracks, count, index);
    keel_nav_now_playing();
}

void keel_action_shuffle_tracks(const int32_t * tracks, int32_t count)
{
    if(count <= 0) return;
    keel_player_play_shuffled(tracks, count);
    keel_nav_now_playing();
}

/* Tracks of an album (or of every album by an artist) that still exist. */
static int32_t collect(int32_t album, int32_t artist, int32_t ** out)
{
    const keel_library_t * lib = keel_library();
    int32_t * t = malloc(sizeof(int32_t) * (size_t)lib->track_count);
    int32_t n = 0;
    for(int32_t a = 0; a < lib->album_count; a++) {
        if(album >= 0 && a != album) continue;
        if(artist >= 0 && lib->albums[a].artist != artist) continue;
        for(int32_t i = 0; i < lib->albums[a].track_count; i++) {
            int32_t ti = lib->albums[a].first_track + i;
            if(!keel_library_track_deleted(ti)) t[n++] = ti;
        }
    }
    *out = t;
    return n;
}

void keel_action_play_album(int32_t album, bool shuffle)
{
    int32_t * t;
    int32_t n = collect(album, -1, &t);
    if(shuffle) keel_action_shuffle_tracks(t, n);
    else keel_action_play_tracks(t, n, 0);
    free(t);
}

void keel_action_play_artist(int32_t artist, bool shuffle)
{
    int32_t * t;
    int32_t n = collect(-1, artist, &t);
    if(shuffle) keel_action_shuffle_tracks(t, n);
    else keel_action_play_tracks(t, n, 0);
    free(t);
}

int32_t keel_visible_tracks(int32_t ** out)
{
    const keel_library_t * lib = keel_library();
    int32_t * t = malloc(sizeof(int32_t) * (size_t)lib->track_count);
    int32_t n = 0;
    for(int32_t i = 0; i < lib->track_count; i++)
        if(!keel_library_track_deleted(lib->tracks_by_title[i])) t[n++] = lib->tracks_by_title[i];
    *out = t;
    return n;
}

void keel_action_shuffle_all(void)
{
    int32_t * t;
    int32_t n = keel_visible_tracks(&t);
    keel_action_shuffle_tracks(t, n);
    free(t);
}

void keel_action_go_album(int32_t album)
{
    keel_nav_push(keel_scr_album, PTR(album));
}

void keel_action_go_artist(int32_t artist)
{
    keel_nav_push(keel_scr_artist, PTR(artist));
}

/* ---- Menu callbacks ----------------------------------------------------- */

static void play_next_cb(void * u) { keel_player_play_next(IDX(u)); }
static void go_album_cb(void * u) { keel_action_go_album(IDX(u)); }
static void go_artist_cb(void * u) { keel_action_go_artist(IDX(u)); }
static void play_album_cb(void * u) { keel_action_play_album(IDX(u), false); }
static void shuffle_album_cb(void * u) { keel_action_play_album(IDX(u), true); }
static void play_artist_cb(void * u) { keel_action_play_artist(IDX(u), false); }
static void shuffle_artist_cb(void * u) { keel_action_play_artist(IDX(u), true); }
static void shuffle_all_cb(void * u) { keel_action_shuffle_all(); }
static void now_playing_cb(void * u) { keel_nav_now_playing(); }

static void play_song_cb(void * u)
{
    int32_t t = IDX(u);
    keel_action_play_tracks(&t, 1, 0);
}

static void like_song_cb(void * u)
{
    keel_library_set_track_liked(IDX(u), !keel_library_track_liked(IDX(u)));
    keel_player_library_changed();  /* now playing shows the new state */
}

static void like_album_cb(void * u)
{
    keel_library_set_album_liked(IDX(u), !keel_library_album_liked(IDX(u)));
}

static void liked_songs_cb(void * u)
{
    keel_nav_push(keel_scr_playlist, PTR(keel_library()->playlist_count));
}

static void delete_song_cb(void * u)
{
    int32_t t = IDX(u);
    int32_t album = keel_library()->tracks[t].album;
    keel_library_delete_track(t);
    keel_player_library_changed();
    hide_origin();
    /* Deleting an album's last track deletes the album: leave its page. */
    if(keel_library_album_deleted(album) && keel_nav_showing(keel_scr_album, PTR(album))) keel_nav_back();
}

static void delete_album_cb(void * u)
{
    keel_library_delete_album(IDX(u));
    keel_player_library_changed();
    hide_origin();
}

static void delete_album_page_cb(void * u)
{
    delete_album_cb(u);
    keel_nav_back();
}

static const char * delete_note = "removes it from this player's library.";

static void ask_delete_song_cb(void * u)
{
    char q[160];
    snprintf(q, sizeof(q), "delete “%s”?", keel_library()->tracks[IDX(u)].title);
    keel_menu_confirm("Delete song", q, delete_note, "delete", delete_song_cb, u);
}

static void ask_delete_album_cb(void * u)
{
    char q[160];
    snprintf(q, sizeof(q), "delete “%s”?", keel_library()->albums[IDX(u)].title);
    keel_menu_confirm("Delete album", q, delete_note, "delete", delete_album_cb, u);
}

static void ask_delete_album_page_cb(void * u)
{
    char q[160];
    snprintf(q, sizeof(q), "delete “%s”?", keel_library()->albums[IDX(u)].title);
    keel_menu_confirm("Delete album", q, delete_note, "delete", delete_album_page_cb, u);
}

void keel_action_ask_delete_song(int32_t t)
{
    set_origin(NULL);
    ask_delete_song_cb(PTR(t));
}

void keel_action_ask_delete_album(int32_t a)
{
    set_origin(NULL);
    ask_delete_album_cb(PTR(a));
}

/* ---- Item menus ------------------------------------------------------- */

void keel_menu_song(int32_t t, lv_obj_t * origin)
{
    const keel_library_t * lib = keel_library();
    const keel_track_t * tr = &lib->tracks[t];
    set_origin(origin);
    keel_menu_item_t items[] = {
        {"play", play_song_cb, PTR(t)},
        {"play next", play_next_cb, PTR(t)},
        {keel_library_track_liked(t) ? "unlike" : "like", like_song_cb, PTR(t)},
        {"go to album", go_album_cb, PTR(tr->album)},
        {"go to artist", go_artist_cb, PTR(lib->albums[tr->album].artist)},
        {"delete", ask_delete_song_cb, PTR(t)},
    };
    keel_menu_open("Song", tr->title, items, sizeof(items) / sizeof(items[0]));
}

void keel_menu_album(int32_t a, lv_obj_t * origin)
{
    const keel_album_t * al = &keel_library()->albums[a];
    set_origin(origin);
    keel_menu_item_t items[] = {
        {"play", play_album_cb, PTR(a)},
        {"shuffle", shuffle_album_cb, PTR(a)},
        {keel_library_album_liked(a) ? "unlike" : "like", like_album_cb, PTR(a)},
        {"go to artist", go_artist_cb, PTR(al->artist)},
        {"delete", ask_delete_album_cb, PTR(a)},
    };
    keel_menu_open("Album", al->title, items, sizeof(items) / sizeof(items[0]));
}

void keel_menu_artist(int32_t ar, lv_obj_t * origin)
{
    set_origin(origin);
    keel_menu_item_t items[] = {
        {"play all", play_artist_cb, PTR(ar)},
        {"shuffle", shuffle_artist_cb, PTR(ar)},
    };
    keel_menu_open("Artist", keel_library()->artists[ar].name, items, sizeof(items) / sizeof(items[0]));
}

void keel_long_press_song(lv_obj_t * obj, void * user) { keel_menu_song(IDX(user), obj); }
void keel_long_press_album(lv_obj_t * obj, void * user) { keel_menu_album(IDX(user), obj); }
void keel_long_press_artist(lv_obj_t * obj, void * user) { keel_menu_artist(IDX(user), obj); }

/* ---- Screen menus ------------------------------------------------------ */

void keel_menu_album_page(int32_t a)
{
    const keel_album_t * al = &keel_library()->albums[a];
    set_origin(NULL);
    keel_menu_item_t items[] = {
        {"play", play_album_cb, PTR(a)},
        {"shuffle", shuffle_album_cb, PTR(a)},
        {keel_library_album_liked(a) ? "unlike album" : "like album", like_album_cb, PTR(a)},
        {"go to artist", go_artist_cb, PTR(al->artist)},
        {"delete album", ask_delete_album_page_cb, PTR(a)},
    };
    keel_menu_open("Album", al->title, items, sizeof(items) / sizeof(items[0]));
}

void keel_menu_now_playing(void)
{
    int32_t t = keel_player_track();
    if(t < 0) return;
    const keel_library_t * lib = keel_library();
    const keel_track_t * tr = &lib->tracks[t];
    set_origin(NULL);
    keel_menu_item_t items[] = {
        {keel_library_track_liked(t) ? "unlike" : "like", like_song_cb, PTR(t)},
        {"go to album", go_album_cb, PTR(tr->album)},
        {"go to artist", go_artist_cb, PTR(lib->albums[tr->album].artist)},
        {"delete", ask_delete_song_cb, PTR(t)},
    };
    keel_menu_open("Now playing", tr->title, items, sizeof(items) / sizeof(items[0]));
}

void keel_menu_library(const char * title)
{
    keel_menu_item_t items[3];
    int32_t n = 0;
    if(keel_player_active()) items[n++] = (keel_menu_item_t){"now playing", now_playing_cb, NULL};
    items[n++] = (keel_menu_item_t){"shuffle all", shuffle_all_cb, NULL};
    if(keel_library_liked_tracks(NULL, 0) > 0) items[n++] = (keel_menu_item_t){"liked songs", liked_songs_cb, NULL};
    set_origin(NULL);
    keel_menu_open("Menu", title, items, n);
}
