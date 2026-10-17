#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "library.h"
#include "nav.h"
#include "player.h"
#include "screens.h"
#include "widgets.h"

typedef struct {
    int32_t first_track;
    int32_t count;
    lv_obj_t * markers[];  /* one per row, shown on the playing track */
} album_t;

static void markers_update(lv_observer_t * obs, lv_subject_t * s)
{
    album_t * a = lv_observer_get_user_data(obs);
    int32_t playing = keel_player_track();
    for(int32_t i = 0; i < a->count; i++)
        lv_obj_set_hidden(a->markers[i], a->first_track + i != playing);
}

static void track_cb(lv_event_t * e)
{
    const keel_library_t * lib = keel_library();
    const keel_track_t * t = &lib->tracks[(intptr_t)lv_event_get_user_data(e)];
    keel_player_play_album(t->album, t->number - 1);
    keel_nav_push(keel_scr_now_playing, NULL);
}

static void free_cb(lv_event_t * e)
{
    free(lv_event_get_user_data(e));
}

void keel_scr_album(lv_obj_t * body, void * arg)
{
    const keel_library_t * lib = keel_library();
    int32_t ai = LV_CLAMP(0, (int32_t)(intptr_t)arg, lib->album_count - 1);
    const keel_album_t * al = &lib->albums[ai];

    lv_obj_t * col = lv_obj_create(body);
    lv_obj_add_style(col, keel_style_plain(), 0);
    lv_obj_set_size(col, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scroll_dir(col, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(col, LV_SCROLLBAR_MODE_OFF);
    /* Room for the last track to scroll clear of the screen edge. */
    lv_obj_set_style_pad_bottom(col, keel_space.space_8, 0);

    album_t * a = calloc(1, sizeof(*a) + (size_t)al->track_count * sizeof(lv_obj_t *));
    a->first_track = al->first_track;
    a->count = al->track_count;
    lv_obj_add_event_cb(col, free_cb, LV_EVENT_DELETE, a);

    /* The art is the one thing that is not text, so it gets air above. */
    lv_obj_t * art = keel_cover(col, ai, KEEL_ART_LARGE);
    lv_obj_set_style_margin_top(art, keel_space.space_2, 0);
    lv_obj_set_style_margin_left(art, keel_space.margin, 0);
    keel_nav_stagger(art, 0);

    char over[32];
    snprintf(over, sizeof(over), "Album · %d", (int)al->year);
    /* The album name is `heading`, not `title`: compact, so long names like
     * "draining love story (in the eyes of cynthoni)" take two lines, not
     * four, and the track list starts sooner. */
    lv_obj_t * head = keel_header(col, over, al->title, KEEL_TYPE_HEADING);
    lv_obj_set_style_margin_top(head, keel_space.space_2, 0);
    keel_nav_stagger(head, 0);

    /* Count and length ride inline: a data line under small would stack two
     * 21px styles (rule 5). */
    char tm[16], meta[160];
    keel_format_time(tm, sizeof(tm), al->duration);
    snprintf(meta, sizeof(meta), "%s · %d %s · %s", lib->artists[al->artist].name, (int)al->track_count,
             al->track_count == 1 ? "song" : "songs", tm);
    lv_obj_t * artist = keel_text(col, KEEL_TYPE_SMALL, meta);
    lv_obj_set_width(artist, LV_PCT(100));
    lv_obj_set_style_pad_hor(artist, keel_space.margin, 0);
    lv_obj_set_style_text_color(artist, keel_color.muted, 0);
    lv_obj_set_style_margin_bottom(artist, keel_space.space_4, 0);
    keel_nav_stagger(artist, 0);

    for(int32_t i = 0; i < al->track_count; i++) {
        int32_t ti = al->first_track + i;
        char dur[16];
        keel_format_time(dur, sizeof(dur), lib->tracks[ti].duration);
        lv_obj_t * row = keel_row(col, lib->tracks[ti].title, NULL, dur, track_cb, (void *)(intptr_t)ti);

        /* Selected marker: accent square in the left margin. The row text
         * keeps its weight 600 and color, so accent is decoration only. */
        lv_obj_t * m = keel_box(row);
        lv_obj_set_size(m, keel_space.space_2, keel_space.space_2);
        lv_obj_set_style_bg_color(m, keel_color.accent, 0);
        lv_obj_set_style_bg_opa(m, LV_OPA_COVER, 0);
        lv_obj_set_floating(m, true);
        lv_obj_align(m, LV_ALIGN_LEFT_MID, keel_space.space_2, 0);
        lv_obj_set_style_translate_x(m, -keel_space.margin, 0);
        a->markers[i] = m;

        keel_nav_stagger(row, i + 1);
    }

    lv_subject_add_observer_obj(keel_player_subject(), markers_update, col, a);
}
