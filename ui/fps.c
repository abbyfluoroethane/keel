/* Frame timing. Measures each LVGL refresh (render + flush) with a
 * monotonic clock and, with --fps, shows the last second in a corner in
 * the `data` style. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "keel.h"
#include "theme.h"

#define MAX_SAMPLES 20000
/* One 60 Hz period. LVGL skips a refresh when nothing changed, so gaps
 * between frames say nothing about speed; a frame longer than this does. */
#define BUDGET_MS 16.7

static uint64_t refr_start_us;
static uint64_t flush_start_us;
static uint64_t frame_flush_us;
static double flush_sum_ms;

static float samples[MAX_SAMPLES];
static uint32_t sample_count;
static uint32_t over_budget;

/* Last second, for the overlay. */
static uint32_t sec_frames;
static double sec_sum_ms;
static double sec_max_ms;
static lv_obj_t * label;

static uint64_t now_us(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000u + (uint64_t)ts.tv_nsec / 1000u;
}

static void refr_start_cb(lv_event_t * e)
{
    refr_start_us = now_us();
    frame_flush_us = 0;
}

static void refr_ready_cb(lv_event_t * e)
{
    double ms = (double)(now_us() - refr_start_us) / 1000.0;
    if(sample_count < MAX_SAMPLES) {
        samples[sample_count++] = (float)ms;
        flush_sum_ms += (double)frame_flush_us / 1000.0;
    }
    if(ms > BUDGET_MS) over_budget++;
    sec_frames++;
    sec_sum_ms += ms;
    if(ms > sec_max_ms) sec_max_ms = ms;
}

static void flush_start_cb(lv_event_t * e)
{
    flush_start_us = now_us();
}

static void flush_finish_cb(lv_event_t * e)
{
    frame_flush_us += now_us() - flush_start_us;
}

static void overlay_tick(lv_timer_t * t)
{
    if(!label) return;
    char buf[48];
    if(sec_frames <= 1) snprintf(buf, sizeof(buf), "idle");
    else snprintf(buf, sizeof(buf), "%.1f ms  max %.1f  %u fps", sec_sum_ms / sec_frames, sec_max_ms, sec_frames);
    sec_frames = 0;
    sec_sum_ms = 0;
    sec_max_ms = 0;
    lv_label_set_text(label, buf);
}

void keel_fps_init(bool show)
{
    lv_display_t * d = lv_display_get_default();
    lv_display_add_event_cb(d, refr_start_cb, LV_EVENT_REFR_START, NULL);
    lv_display_add_event_cb(d, refr_ready_cb, LV_EVENT_REFR_READY, NULL);
    lv_display_add_event_cb(d, flush_start_cb, LV_EVENT_FLUSH_START, NULL);
    lv_display_add_event_cb(d, flush_finish_cb, LV_EVENT_FLUSH_FINISH, NULL);
    if(!show) return;

    /* System layer: above screens and the status bar. A `base` backing so
     * the numbers stay readable over cover art. */
    label = keel_text(lv_layer_sys(), KEEL_TYPE_DATA, "idle");
    lv_obj_set_style_bg_color(label, keel_color.base, 0);
    lv_obj_set_style_bg_opa(label, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_hor(label, keel_space.space_1, 0);
    lv_obj_align(label, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_timer_create(overlay_tick, 1000, NULL);
}

void keel_frame_stats_reset(void)
{
    sample_count = 0;
    flush_sum_ms = 0;
    over_budget = 0;
}

static int cmp_float(const void * a, const void * b)
{
    float x = *(const float *)a, y = *(const float *)b;
    return (x > y) - (x < y);
}

void keel_frame_stats_get(keel_frame_stats_t * out)
{
    memset(out, 0, sizeof(*out));
    out->frames = sample_count;
    if(!sample_count) return;
    static float sorted[MAX_SAMPLES];
    memcpy(sorted, samples, sizeof(float) * sample_count);
    qsort(sorted, sample_count, sizeof(float), cmp_float);
    double sum = 0;
    for(uint32_t i = 0; i < sample_count; i++) sum += sorted[i];
    out->avg_ms = sum / sample_count;
    out->avg_flush_ms = flush_sum_ms / sample_count;
    out->max_ms = sorted[sample_count - 1];
    out->p95_ms = sorted[(uint32_t)((sample_count - 1) * 0.95)];
    out->p99_ms = sorted[(uint32_t)((sample_count - 1) * 0.99)];
    out->over_budget = over_budget;
}
