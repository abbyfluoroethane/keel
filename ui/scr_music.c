#include <stdint.h>
#include <stdio.h>

#include "library.h"
#include "nav.h"
#include "player.h"
#include "screens.h"
#include "widgets.h"

#define STAGGER_ROWS 10

static const char * plural(int32_t n, const char * one, const char * many)
{
    return n == 1 ? one : many;
}

/* A vertical scroll column that fills the screen body. */
static lv_obj_t * scroll_column(lv_obj_t * body)
{
    lv_obj_t * col = lv_obj_create(body);
    lv_obj_add_style(col, keel_style_plain(), 0);
    lv_obj_set_size(col, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scroll_dir(col, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(col, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_clickable(col, false);
    lv_obj_set_style_pad_bottom(col, keel_space.space_8, 0);
    return col;
}

/* Room under the header strip and past the last row. */
static void pane_padding(lv_obj_t * pane)
{
    lv_obj_set_style_pad_top(pane, keel_space.space_3, 0);
    lv_obj_set_style_pad_bottom(pane, keel_space.space_8, 0);
}

static void track_secondary(const keel_track_t * tr, const char ** artist)
{
    const keel_library_t * lib = keel_library();
    *artist = lib->artists[lib->albums[tr->album].artist].name;
}

static void open_album_cb(lv_event_t * e)
{
    keel_nav_push(keel_scr_album, lv_event_get_user_data(e));
}

static void open_artist_cb(lv_event_t * e)
{
    keel_nav_push(keel_scr_artist, lv_event_get_user_data(e));
}

static void open_playlist_cb(lv_event_t * e)
{
    keel_nav_push(keel_scr_playlist, lv_event_get_user_data(e));
}

static void play_song_cb(lv_event_t * e)
{
    const keel_library_t * lib = keel_library();
    keel_player_play(lib->tracks_by_title, lib->track_count, (int32_t)(intptr_t)lv_event_get_user_data(e));
    keel_nav_push(keel_scr_now_playing, NULL);
}

static void play_playlist_cb(lv_event_t * e)
{
    /* Playlist and position share one pointer-sized user value. */
    intptr_t v = (intptr_t)lv_event_get_user_data(e);
    const keel_playlist_t * pl = &keel_library()->playlists[v >> 16];
    keel_player_play(pl->tracks, pl->track_count, (int32_t)(v & 0xffff));
    keel_nav_push(keel_scr_now_playing, NULL);
}

static lv_obj_t * track_row(lv_obj_t * parent, const keel_track_t * tr, lv_event_cb_t cb, intptr_t user)
{
    const char * artist;
    char time[16];
    track_secondary(tr, &artist);
    keel_format_time(time, sizeof(time), tr->duration);
    return keel_row(parent, tr->title, artist, time, cb, (void *)user);
}

/* ---- Pivot panes ------------------------------------------------------ */

static void build_artists(lv_obj_t * pane, bool stagger)
{
    const keel_library_t * lib = keel_library();
    pane_padding(pane);
    for(int32_t i = 0; i < lib->artist_count; i++) {
        const keel_artist_t * a = &lib->artists[i];
        char sec[48];
        snprintf(sec, sizeof(sec), "%d %s \xC2\xB7 %d %s", (int)a->album_count,
                 plural(a->album_count, "album", "albums"), (int)a->track_count,
                 plural(a->track_count, "song", "songs"));
        lv_obj_t * r = keel_row(pane, a->name, sec, NULL, open_artist_cb, (void *)(intptr_t)i);
        if(stagger && i < STAGGER_ROWS) keel_nav_stagger(r, i);
    }
}

static void build_albums(lv_obj_t * pane, bool stagger)
{
    const keel_library_t * lib = keel_library();
    pane_padding(pane);

    lv_obj_t * grid = keel_box(pane);
    lv_obj_set_width(grid, LV_PCT(100));
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_left(grid, keel_space.margin, 0);
    lv_obj_set_style_pad_column(grid, keel_space.tile_gap, 0);
    lv_obj_set_style_pad_row(grid, keel_space.tile_gap, 0);

    for(int32_t i = 0; i < lib->album_count; i++) {
        /* The tile is the cover itself: art only, as on the Zune. */
        lv_obj_t * t = keel_cover(grid, i, KEEL_ART_TILE);
        lv_obj_set_clickable(t, true);
        lv_obj_add_style(t, keel_style_focus_ring(), LV_STATE_FOCUS_KEY);
        lv_obj_set_style_opa(t, LV_OPA_60, LV_STATE_PRESSED);
        lv_obj_set_scroll_chain(t, true);
        lv_obj_add_event_cb(t, open_album_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        keel_nav_focusable(t);
        if(stagger && i < STAGGER_ROWS) keel_nav_stagger(t, i);
    }
}

static void build_songs(lv_obj_t * pane, bool stagger)
{
    const keel_library_t * lib = keel_library();
    pane_padding(pane);
    for(int32_t i = 0; i < lib->track_count; i++) {
        lv_obj_t * r = track_row(pane, &lib->tracks[lib->tracks_by_title[i]], play_song_cb, i);
        if(stagger && i < STAGGER_ROWS) keel_nav_stagger(r, i);
    }
}

static void build_playlists(lv_obj_t * pane, bool stagger)
{
    const keel_library_t * lib = keel_library();
    pane_padding(pane);
    for(int32_t i = 0; i < lib->playlist_count; i++) {
        const keel_playlist_t * pl = &lib->playlists[i];
        char n[24];
        snprintf(n, sizeof(n), "%d %s", (int)pl->track_count, plural(pl->track_count, "song", "songs"));
        lv_obj_t * r = keel_row(pane, pl->title, n, NULL, open_playlist_cb, (void *)(intptr_t)i);
        if(stagger && i < STAGGER_ROWS) keel_nav_stagger(r, i);
    }
}

void keel_scr_music(lv_obj_t * body, void * arg)
{
    static const char * const names[4] = {"artists", "albums", "songs", "playlists"};
    int32_t * st = keel_nav_state();
    int32_t initial = (st && *st >= 0) ? *st : (int32_t)(intptr_t)arg;
    initial = LV_CLAMP(0, initial, 3);

    lv_obj_t * pv = keel_pivot(body, NULL, names, 4, initial);
    void (*const builders[4])(lv_obj_t *, bool) = {build_artists, build_albums, build_songs, build_playlists};
    for(int32_t i = 0; i < 4; i++) builders[i](keel_pivot_pane(pv, i), i == initial);
}

/* ---- Artist ----------------------------------------------------------- */

void keel_scr_artist(lv_obj_t * body, void * arg)
{
    const keel_library_t * lib = keel_library();
    int32_t ai = (int32_t)(intptr_t)arg;
    lv_obj_t * col = scroll_column(body);
    lv_obj_t * h = keel_header(col, "Artist", lib->artists[ai].name, KEEL_TYPE_TITLE);
    lv_obj_set_style_margin_bottom(h, keel_space.space_3, 0);

    int32_t n = 0;
    for(int32_t i = 0; i < lib->album_count; i++) {
        const keel_album_t * al = &lib->albums[i];
        if(al->artist != ai) continue;
        char sec[48];
        snprintf(sec, sizeof(sec), "%d \xC2\xB7 %d %s", (int)al->year, (int)al->track_count,
                 plural(al->track_count, "song", "songs"));
        lv_obj_t * r = keel_row(col, al->title, sec, NULL, open_album_cb, (void *)(intptr_t)i);
        if(n < STAGGER_ROWS) keel_nav_stagger(r, n);
        n++;
    }
}

/* ---- Playlist --------------------------------------------------------- */

void keel_scr_playlist(lv_obj_t * body, void * arg)
{
    const keel_library_t * lib = keel_library();
    int32_t pi = (int32_t)(intptr_t)arg;
    const keel_playlist_t * pl = &lib->playlists[pi];
    lv_obj_t * col = scroll_column(body);
    lv_obj_t * h = keel_header(col, "Playlist", pl->title, KEEL_TYPE_TITLE);
    lv_obj_set_style_margin_bottom(h, keel_space.space_3, 0);

    for(int32_t i = 0; i < pl->track_count; i++) {
        lv_obj_t * r = track_row(col, &lib->tracks[pl->tracks[i]], play_playlist_cb, ((intptr_t)pi << 16) | i);
        if(i < STAGGER_ROWS) keel_nav_stagger(r, i);
    }
}
