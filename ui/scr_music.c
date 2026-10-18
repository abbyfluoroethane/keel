#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "actions.h"
#include "library.h"
#include "menu.h"
#include "nav.h"
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

/* The track list the songs pane or the playlist page shows. Taps play it
 * from the tapped row; tracks deleted since are left out, as the rows are
 * hidden. Only one of those two lists is on screen at a time. */
static int32_t * shown;
static int32_t shown_count;

static void shown_set(int32_t * tracks, int32_t count)
{
    free(shown);
    shown = tracks;
    shown_count = count;
}

/* Copy of `shown` without deleted tracks. Returns the count and, when
 * `track` >= 0, the position of that track in `position`. */
static int32_t shown_live(int32_t * out, int32_t track, int32_t * position)
{
    int32_t n = 0;
    if(position) *position = 0;
    for(int32_t i = 0; i < shown_count; i++) {
        if(keel_library_track_deleted(shown[i])) continue;
        if(shown[i] == track && position) *position = n;
        out[n++] = shown[i];
    }
    return n;
}

static void play_shown(int32_t track, bool shuffle)
{
    int32_t * live = malloc(sizeof(int32_t) * (size_t)(shown_count > 0 ? shown_count : 1));
    int32_t pos;
    int32_t n = shown_live(live, track, &pos);
    if(shuffle) keel_action_shuffle_tracks(live, n);
    else keel_action_play_tracks(live, n, pos);
    free(live);
}

static void play_shown_cb(lv_event_t * e)
{
    play_shown((int32_t)(intptr_t)lv_event_get_user_data(e), false);
}

static lv_obj_t * track_row(lv_obj_t * parent, int32_t track, lv_event_cb_t cb)
{
    const keel_track_t * tr = &keel_library()->tracks[track];
    const char * artist;
    char time[16];
    track_secondary(tr, &artist);
    keel_format_time(time, sizeof(time), tr->duration);
    lv_obj_t * r = keel_row(parent, tr->title, artist, time, cb, (void *)(intptr_t)track);
    keel_menu_on_long_press(r, keel_long_press_song, (void *)(intptr_t)track);
    return r;
}

static void library_menu_cb(void * user)
{
    LV_UNUSED(user);
    keel_menu_library("music");
}

/* ---- Pivot panes ------------------------------------------------------ */

static void build_artists(lv_obj_t * pane, bool stagger)
{
    const keel_library_t * lib = keel_library();
    pane_padding(pane);
    int32_t n = 0;
    for(int32_t i = 0; i < lib->artist_count; i++) {
        int32_t albums, tracks;
        keel_library_artist_counts(i, &albums, &tracks);
        if(albums == 0) continue;
        char sec[48];
        snprintf(sec, sizeof(sec), "%d %s \xC2\xB7 %d %s", (int)albums, plural(albums, "album", "albums"),
                 (int)tracks, plural(tracks, "song", "songs"));
        lv_obj_t * r = keel_row(pane, lib->artists[i].name, sec, NULL, open_artist_cb, (void *)(intptr_t)i);
        keel_menu_on_long_press(r, keel_long_press_artist, (void *)(intptr_t)i);
        if(stagger && n < STAGGER_ROWS) keel_nav_stagger(r, n);
        n++;
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

    int32_t n = 0;
    for(int32_t i = 0; i < lib->album_count; i++) {
        if(keel_library_album_deleted(i)) continue;
        /* The tile is the cover itself: art only, as on the Zune. */
        lv_obj_t * t = keel_cover(grid, i, KEEL_ART_TILE);
        lv_obj_set_clickable(t, true);
        lv_obj_add_style(t, keel_style_focus_ring(), LV_STATE_FOCUS_KEY);
        lv_obj_set_style_opa(t, LV_OPA_60, LV_STATE_PRESSED);
        lv_obj_set_scroll_chain(t, true);
        lv_obj_add_event_cb(t, open_album_cb, LV_EVENT_SHORT_CLICKED, (void *)(intptr_t)i);
        keel_menu_on_long_press(t, keel_long_press_album, (void *)(intptr_t)i);
        keel_nav_focusable(t);
        if(stagger && n < STAGGER_ROWS) keel_nav_stagger(t, n);
        n++;
    }
}

static void build_songs(lv_obj_t * pane, bool stagger)
{
    int32_t * tracks;
    int32_t count = keel_visible_tracks(&tracks);
    shown_set(tracks, count);
    pane_padding(pane);
    for(int32_t i = 0; i < count; i++) {
        lv_obj_t * r = track_row(pane, tracks[i], play_shown_cb);
        if(stagger && i < STAGGER_ROWS) keel_nav_stagger(r, i);
    }
}

static int32_t playlist_live_count(const keel_playlist_t * pl)
{
    int32_t n = 0;
    for(int32_t i = 0; i < pl->track_count; i++) {
        if(!keel_library_track_deleted(pl->tracks[i])) n++;
    }
    return n;
}

static void songs_text(char * buf, size_t size, int32_t n)
{
    snprintf(buf, size, "%d %s", (int)n, plural(n, "song", "songs"));
}

static void build_playlists(lv_obj_t * pane, bool stagger)
{
    const keel_library_t * lib = keel_library();
    pane_padding(pane);
    int32_t n = 0;
    char txt[24];

    int32_t liked = keel_library_liked_tracks(NULL, 0);
    if(liked > 0) {
        songs_text(txt, sizeof(txt), liked);
        lv_obj_t * r = keel_row(pane, "liked songs", txt, NULL, open_playlist_cb, (void *)(intptr_t)lib->playlist_count);
        if(stagger) keel_nav_stagger(r, n);
        n++;
    }
    for(int32_t i = 0; i < lib->playlist_count; i++) {
        songs_text(txt, sizeof(txt), playlist_live_count(&lib->playlists[i]));
        lv_obj_t * r = keel_row(pane, lib->playlists[i].title, txt, NULL, open_playlist_cb, (void *)(intptr_t)i);
        if(stagger && n < STAGGER_ROWS) keel_nav_stagger(r, n);
        n++;
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
    keel_nav_set_menu(library_menu_cb, NULL);
}

/* ---- Artist ----------------------------------------------------------- */

static void artist_play_cb(void * user)
{
    keel_action_play_artist((int32_t)(intptr_t)user, false);
}

static void artist_shuffle_cb(void * user)
{
    keel_action_play_artist((int32_t)(intptr_t)user, true);
}

static void artist_menu_cb(void * user)
{
    static const char * const labels[2] = {"play all", "shuffle"};
    keel_menu_item_t items[2];
    for(int32_t i = 0; i < 2; i++) items[i] = (keel_menu_item_t){labels[i], NULL, NULL};
    items[0].cb = artist_play_cb;
    items[1].cb = artist_shuffle_cb;
    items[0].user = items[1].user = user;
    keel_menu_open("Artist", keel_library()->artists[(int32_t)(intptr_t)user].name, items, 2);
}

void keel_scr_artist(lv_obj_t * body, void * arg)
{
    const keel_library_t * lib = keel_library();
    int32_t ai = (int32_t)(intptr_t)arg;
    lv_obj_t * col = scroll_column(body);
    lv_obj_t * h = keel_header(col, "Artist", lib->artists[ai].name, KEEL_TYPE_TITLE);
    lv_obj_set_style_margin_bottom(h, keel_space.space_3, 0);
    keel_nav_set_menu(artist_menu_cb, arg);

    int32_t n = 0;
    for(int32_t i = 0; i < lib->album_count; i++) {
        const keel_album_t * al = &lib->albums[i];
        if(al->artist != ai || keel_library_album_deleted(i)) continue;
        int32_t tracks;
        keel_library_album_counts(i, &tracks, NULL);
        char sec[48];
        snprintf(sec, sizeof(sec), "%d \xC2\xB7 %d %s", (int)al->year, (int)tracks, plural(tracks, "song", "songs"));
        lv_obj_t * r = keel_row(col, al->title, sec, NULL, open_album_cb, (void *)(intptr_t)i);
        keel_menu_on_long_press(r, keel_long_press_album, (void *)(intptr_t)i);
        if(n < STAGGER_ROWS) keel_nav_stagger(r, n);
        n++;
    }
}

/* ---- Playlist --------------------------------------------------------- */

static const char * playlist_title(int32_t pi)
{
    const keel_library_t * lib = keel_library();
    return pi == lib->playlist_count ? "liked songs" : lib->playlists[pi].title;
}

static void playlist_play_cb(void * user)
{
    LV_UNUSED(user);
    play_shown(-1, false);
}

static void playlist_shuffle_cb(void * user)
{
    LV_UNUSED(user);
    play_shown(-1, true);
}

static void playlist_menu_cb(void * user)
{
    keel_menu_item_t items[2] = {{"play", playlist_play_cb, NULL}, {"shuffle", playlist_shuffle_cb, NULL}};
    keel_menu_open("Playlist", playlist_title((int32_t)(intptr_t)user), items, 2);
}

void keel_scr_playlist(lv_obj_t * body, void * arg)
{
    const keel_library_t * lib = keel_library();
    int32_t pi = (int32_t)(intptr_t)arg;
    int32_t * tracks;
    int32_t count;
    if(pi == lib->playlist_count) {
        count = keel_library_liked_tracks(NULL, 0);
        tracks = malloc(sizeof(int32_t) * (size_t)(count > 0 ? count : 1));
        keel_library_liked_tracks(tracks, count);
    } else {
        const keel_playlist_t * pl = &lib->playlists[pi];
        tracks = malloc(sizeof(int32_t) * (size_t)(pl->track_count > 0 ? pl->track_count : 1));
        count = 0;
        for(int32_t i = 0; i < pl->track_count; i++) {
            if(!keel_library_track_deleted(pl->tracks[i])) tracks[count++] = pl->tracks[i];
        }
    }
    shown_set(tracks, count);

    lv_obj_t * col = scroll_column(body);
    lv_obj_t * h = keel_header(col, "Playlist", playlist_title(pi), KEEL_TYPE_TITLE);
    lv_obj_set_style_margin_bottom(h, keel_space.space_3, 0);
    keel_nav_set_menu(playlist_menu_cb, arg);

    for(int32_t i = 0; i < count; i++) {
        lv_obj_t * r = track_row(col, tracks[i], play_shown_cb);
        if(i < STAGGER_ROWS) keel_nav_stagger(r, i);
    }
}
