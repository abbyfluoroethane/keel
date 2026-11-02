#include "player.h"

#include <stdlib.h>
#include <string.h>

#include "library.h"

#define TICK_MS 250

static int32_t * queue;
static int32_t queue_len;
static int32_t queue_pos = -1;
static bool playing;
static uint32_t position_ms;
static uint32_t last_tick;

static lv_subject_t * state_subject; /* bumped on any track or play state change */
static lv_subject_t * pos_subject;   /* position in ms */
static lv_timer_t * timer;

static void changed(void)
{
    lv_subject_set_int(state_subject, lv_subject_get_int(state_subject) + 1);
    lv_subject_set_int(pos_subject, (int32_t)position_ms);
}

static void tick_cb(lv_timer_t * t)
{
    uint32_t now = lv_tick_get();
    uint32_t dt = lv_tick_diff(now, last_tick);
    last_tick = now;
    if(!playing || queue_pos < 0) return;
    position_ms += dt;
    const keel_track_t * tr = &keel_library()->tracks[queue[queue_pos]];
    if(position_ms >= (uint32_t)tr->duration * 1000) {
        keel_player_next();
        return;
    }
    lv_subject_set_int(pos_subject, (int32_t)position_ms);
}

void keel_player_init(void)
{
    state_subject = lv_subject_create(LV_SUBJECT_TYPE_INT);
    pos_subject = lv_subject_create(LV_SUBJECT_TYPE_INT);
    last_tick = lv_tick_get();
    timer = lv_timer_create(tick_cb, TICK_MS, NULL);
}

void keel_player_play(const int32_t * tracks, int32_t count, int32_t index)
{
    if(count <= 0) return;
    free(queue);
    queue = malloc(sizeof(int32_t) * (size_t)count);
    memcpy(queue, tracks, sizeof(int32_t) * (size_t)count);
    queue_len = count;
    queue_pos = LV_CLAMP(0, index, count - 1);
    position_ms = 0;
    playing = true;
    last_tick = lv_tick_get();
    changed();
}

void keel_player_play_album(int32_t album, int32_t index)
{
    const keel_album_t * al = &keel_library()->albums[album];
    int32_t * t = malloc(sizeof(int32_t) * (size_t)al->track_count);
    for(int32_t i = 0; i < al->track_count; i++) t[i] = al->first_track + i;
    keel_player_play(t, al->track_count, index);
    free(t);
}

void keel_player_play_shuffled(const int32_t * tracks, int32_t count)
{
    if(count <= 0) return;
    int32_t * t = malloc(sizeof(int32_t) * (size_t)count);
    memcpy(t, tracks, sizeof(int32_t) * (size_t)count);
    for(int32_t i = count - 1; i > 0; i--) {
        int32_t j = rand() % (i + 1);
        int32_t x = t[i];
        t[i] = t[j];
        t[j] = x;
    }
    keel_player_play(t, count, 0);
    free(t);
}

void keel_player_play_next(int32_t track)
{
    if(queue_pos < 0) {
        keel_player_play(&track, 1, 0);
        return;
    }
    queue = realloc(queue, sizeof(int32_t) * (size_t)(queue_len + 1));
    memmove(&queue[queue_pos + 2], &queue[queue_pos + 1], sizeof(int32_t) * (size_t)(queue_len - queue_pos - 1));
    queue[queue_pos + 1] = track;
    queue_len++;
    changed();  /* "track 3 of n" changes */
}

static void stop(void)
{
    queue_pos = -1;
    playing = false;
    position_ms = 0;
    changed();
}

/* Move queue_pos by `dir` until a track that still exists; false if none. */
static bool step(int dir)
{
    for(int32_t i = 0; i < queue_len; i++) {
        queue_pos = (queue_pos + queue_len + dir) % queue_len;
        if(!keel_library_track_deleted(queue[queue_pos])) return true;
    }
    return false;
}

void keel_player_library_changed(void)
{
    if(queue_pos < 0 || !keel_library_track_deleted(queue[queue_pos])) {
        changed();
        return;
    }
    position_ms = 0;
    if(step(1)) changed();
    else stop();
}

void keel_player_toggle(void)
{
    if(queue_pos < 0) return;
    playing = !playing;
    last_tick = lv_tick_get();
    changed();
}

void keel_player_next(void)
{
    if(queue_pos < 0) return;
    position_ms = 0;
    if(step(1)) changed();
    else stop();
}

void keel_player_prev(void)
{
    if(queue_pos < 0) return;
    /* Like most players: restart the track unless it just began. */
    bool back = position_ms < 3000;
    position_ms = 0;
    if(back && !step(-1)) {
        stop();
        return;
    }
    changed();
}

bool keel_player_active(void) { return queue_pos >= 0; }
bool keel_player_playing(void) { return queue_pos >= 0 && playing; }
int32_t keel_player_track(void) { return queue_pos >= 0 ? queue[queue_pos] : -1; }
int32_t keel_player_queue_pos(void) { return queue_pos; }
int32_t keel_player_queue_len(void) { return queue_len; }
uint32_t keel_player_position_ms(void) { return position_ms; }
lv_subject_t * keel_player_subject(void) { return state_subject; }
lv_subject_t * keel_player_pos_subject(void) { return pos_subject; }
