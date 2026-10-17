/* Keel UI entry points for a platform backend.
 *
 * A backend (SDL on the laptop, DRM/evdev on the device) creates an LVGL
 * display of KEEL_HOR_RES x KEEL_VER_RES and its input devices, then calls
 * keel_ui_init(). Nothing in ui/ knows which backend is running.
 */
#ifndef KEEL_H
#define KEEL_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    const char * asset_dir;  /* contains fonts/ and demo/ */
    bool reduced_motion;     /* no parallax, stagger or slides */
    bool show_fps;           /* frame time overlay in a corner */
} keel_options_t;

bool keel_ui_init(const keel_options_t * opts);

/* ---- Demo screens, for --screenshot and --bench ---------------------- */
int keel_demo_count(void);
const char * keel_demo_name(int index);
/* Reset navigation and show demo screen `index` in a typical state. */
void keel_demo_show(int index);

/* ---- Frame statistics (always collected, shown with show_fps) -------- */
typedef struct {
    uint32_t frames;
    double avg_ms;      /* render + flush per frame */
    double avg_flush_ms;  /* of which: handing the frame to the backend */
    double max_ms;
    double p95_ms;
    double p99_ms;
    uint32_t over_budget;  /* frames that took longer than one 60 Hz period */
} keel_frame_stats_t;

void keel_frame_stats_reset(void);
void keel_frame_stats_get(keel_frame_stats_t * out);

#endif /* KEEL_H */
