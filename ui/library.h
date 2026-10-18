/* The demo music library, loaded from assets/demo/library.json.
 *
 * Read-only after load. Indexes are stable for the life of the program.
 * A real media database replaces this later; screens only use this API.
 */
#ifndef KEEL_LIBRARY_H
#define KEEL_LIBRARY_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    const char * title;    /* display title, hand-written lowercase */
    const char * source;   /* title as released (sentence case source) */
    int32_t duration;      /* seconds */
    int32_t album;         /* album index */
    int32_t number;        /* 1-based position on the album */
} keel_track_t;

typedef struct {
    const char * id;
    const char * title;
    const char * source;
    int32_t artist;        /* artist index */
    int32_t year;
    int32_t first_track;   /* index into tracks */
    int32_t track_count;
    int32_t duration;      /* seconds, sum of tracks */
} keel_album_t;

typedef struct {
    const char * id;
    const char * name;     /* display name */
    const char * source;
    int32_t album_count;
    int32_t track_count;
} keel_artist_t;

typedef struct {
    const char * title;
    int32_t * tracks;      /* track indexes */
    int32_t track_count;
} keel_playlist_t;

typedef struct {
    keel_artist_t * artists;
    int32_t artist_count;
    keel_album_t * albums;
    int32_t album_count;
    keel_track_t * tracks;
    int32_t track_count;
    keel_playlist_t * playlists;
    int32_t playlist_count;
    /* Track indexes sorted by display title, for the songs pane. */
    int32_t * tracks_by_title;
} keel_library_t;

bool keel_library_load(const char * asset_dir);
const keel_library_t * keel_library(void);

/* LVGL image source for an album's cover at one of the KEEL_ART_* sizes,
 * or NULL when the file is missing (art is fetched separately, see
 * README). Callers draw a `raised` square instead. */
const char * keel_library_cover(int32_t album, int32_t size);

/* ---- Likes and deletes ------------------------------------------------
 * Kept in memory for this demo (no media database yet), so they reset on
 * restart. A deleted item disappears from every list; deleting an album
 * deletes its tracks, and deleting an album's last track deletes it. */
bool keel_library_track_liked(int32_t track);
void keel_library_set_track_liked(int32_t track, bool liked);
bool keel_library_album_liked(int32_t album);
void keel_library_set_album_liked(int32_t album, bool liked);
bool keel_library_track_deleted(int32_t track);
bool keel_library_album_deleted(int32_t album);
void keel_library_delete_track(int32_t track);
void keel_library_delete_album(int32_t album);

/* Counts that skip deleted items. Any out pointer may be NULL. */
void keel_library_album_counts(int32_t album, int32_t * tracks, int32_t * duration);
void keel_library_artist_counts(int32_t artist, int32_t * albums, int32_t * tracks);
/* Liked, not deleted tracks in library order. Returns how many; writes at
 * most `max` to `out` (which may be NULL to just count). */
int32_t keel_library_liked_tracks(int32_t * out, int32_t max);

/* Decode every album's cover at `size` into LVGL's image cache in the
 * background, one per tick, so a grid scroll never decodes PNGs mid-frame. */
void keel_library_prefetch_covers(int32_t size);

/* Format seconds as m:ss into buf. */
void keel_format_time(char * buf, int32_t size, int32_t seconds);

#endif /* KEEL_LIBRARY_H */
