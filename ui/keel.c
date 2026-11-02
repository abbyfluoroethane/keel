#include "keel.h"

#include <stdint.h>

#include "actions.h"
#include "library.h"
#include "nav.h"
#include "player.h"
#include "screens.h"
#include "theme.h"

void keel_fps_init(bool show);

bool keel_ui_init(const keel_options_t * opts)
{
    if(!keel_theme_init(opts->asset_dir)) return false;
    if(!keel_library_load(opts->asset_dir)) return false;
    keel_player_init();
    keel_nav_init(opts->reduced_motion);
    keel_fps_init(opts->show_fps);
    keel_nav_reset(keel_scr_home, NULL);
    /* Small sizes are cheap to keep (about 1.7MB decoded for the demo):
     * the album grid and the mini player then never decode while moving. */
    keel_library_prefetch_covers(KEEL_ART_THUMB);
    keel_library_prefetch_covers(KEEL_ART_TILE);
    return true;
}

/* ---- Demo screens ----------------------------------------------------- */

typedef struct {
    const char * name;
    void (*show)(void);
} demo_t;

/* Most screens look best with something playing (mini player, now
 * playing). Track 3 of the first album, 1:12 in. */
static void ensure_playing(void)
{
    if(!keel_player_active()) keel_player_play_album(0, 2);
}

static void show_home(void) { ensure_playing(); keel_nav_reset(keel_scr_home, NULL); }
static void show_music(int pane)
{
    ensure_playing();
    keel_nav_reset(keel_scr_home, NULL);
    keel_nav_push(keel_scr_music, (void *)(intptr_t)pane);
}
static void show_artists(void) { show_music(0); }
static void show_albums(void) { show_music(1); }
static void show_songs(void) { show_music(2); }
static void show_playlists(void) { show_music(3); }
static void show_album(void)
{
    show_music(1);
    keel_nav_push(keel_scr_album, (void *)(intptr_t)3);
}
static void show_now_playing(void)
{
    ensure_playing();
    keel_nav_reset(keel_scr_home, NULL);
    keel_nav_push(keel_scr_now_playing, NULL);
}
static void show_radio(void)
{
    keel_nav_reset(keel_scr_home, NULL);
    keel_nav_push(keel_scr_radio, NULL);
}
static void show_settings(void)
{
    keel_nav_reset(keel_scr_home, NULL);
    keel_nav_push(keel_scr_settings, NULL);
}
static void show_about(void)
{
    show_settings();
    keel_nav_push(keel_scr_settings_page, (void *)(intptr_t)3);
}

/* Menus over the album page: the menu button's, a long-pressed song's, and
 * the delete confirmation. */
static int32_t demo_track(void) { return keel_library()->albums[3].first_track + 1; }
static void show_menu_album(void)
{
    show_album();
    keel_menu_album_page(3);
}
static void show_menu_song(void)
{
    show_album();
    keel_menu_song(demo_track(), NULL);
}
static void show_menu_delete(void)
{
    show_album();
    keel_action_ask_delete_song(demo_track());
}

static const demo_t demos[] = {
    {"01-home", show_home},
    {"02-music-artists", show_artists},
    {"03-music-albums", show_albums},
    {"04-music-songs", show_songs},
    {"05-music-playlists", show_playlists},
    {"06-album", show_album},
    {"07-now-playing", show_now_playing},
    {"08-radio", show_radio},
    {"09-settings", show_settings},
    {"10-settings-about", show_about},
    {"11-menu-album", show_menu_album},
    {"12-menu-song", show_menu_song},
    {"13-menu-delete", show_menu_delete},
};

int keel_demo_count(void) { return (int)(sizeof(demos) / sizeof(demos[0])); }
const char * keel_demo_name(int i) { return demos[i].name; }
void keel_demo_show(int i) { demos[i].show(); }
