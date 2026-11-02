/* Fake playback. A timer moves the position; there is no audio yet.
 *
 * Screens watch two LVGL subjects:
 *   keel_player_subject()     changes on track change and play/pause
 *   keel_player_pos_subject() position in ms, updated several times a second
 */
#ifndef KEEL_PLAYER_H
#define KEEL_PLAYER_H

#include "lvgl.h"

/* Play `tracks[index]` with the rest of `tracks` as the queue. */
void keel_player_play(const int32_t * tracks, int32_t count, int32_t index);
/* Play an album from track `index` (0-based on the album). */
void keel_player_play_album(int32_t album, int32_t index);

/* Play `tracks` in a random order. */
void keel_player_play_shuffled(const int32_t * tracks, int32_t count);
/* Queue `track` right after the current one (or play it if nothing plays). */
void keel_player_play_next(int32_t track);
/* Call after deleting from the library: skips a deleted current track and
 * stops when nothing playable is left. */
void keel_player_library_changed(void);

void keel_player_toggle(void);
void keel_player_next(void);
void keel_player_prev(void);

bool keel_player_active(void);   /* a track is loaded */
bool keel_player_playing(void);  /* and not paused */
int32_t keel_player_track(void); /* library track index, -1 when none */
int32_t keel_player_queue_pos(void);
int32_t keel_player_queue_len(void);
uint32_t keel_player_position_ms(void);

lv_subject_t * keel_player_subject(void);
lv_subject_t * keel_player_pos_subject(void);

void keel_player_init(void);

#endif /* KEEL_PLAYER_H */
