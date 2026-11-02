/* SDL2 window, display and input for LVGL.
 *
 * Keel does not use LVGL's own SDL driver: it presents every frame by
 * alpha-blending a full-screen texture through SDL's renderer, which cost
 * about 11 ms per frame here and has no equivalent on the device. This
 * port works like the device will: LVGL renders into one full-frame
 * buffer (DIRECT mode), and flush copies only the changed rectangles to
 * the window surface — the laptop version of a DRM dumb-buffer copy.
 */
#include <stdlib.h>
#include <string.h>

#include <SDL2/SDL.h>

#include "keel.h"
#include "platform.h"
#include "theme.h"

static SDL_Window * window;
static SDL_Surface * frame;  /* wraps the LVGL frame buffer, no copy */
static int scale = 1;
static lv_draw_buf_t * draw_buf;

static lv_point_t mouse_point;
static bool mouse_down;

#define KEY_QUEUE 32
static struct {
    uint32_t key;
    lv_indev_state_t state;
} key_queue[KEY_QUEUE];
static int key_head, key_tail;
static uint32_t last_key;

static void flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map)
{
    SDL_Surface * win = SDL_GetWindowSurface(window);
    if(win) {
        SDL_Rect src = {area->x1, area->y1, lv_area_get_width(area), lv_area_get_height(area)};
        SDL_Rect dst = {src.x * scale, src.y * scale, src.w * scale, src.h * scale};
        if(scale == 1) SDL_BlitSurface(frame, &src, win, &dst);
        else SDL_BlitScaled(frame, &src, win, &dst);
        if(lv_display_flush_is_last(disp)) SDL_UpdateWindowSurface(window);
    }
    lv_display_flush_ready(disp);
}

static void key_push(uint32_t key, lv_indev_state_t state)
{
    int next = (key_head + 1) % KEY_QUEUE;
    if(next == key_tail) return;
    key_queue[key_head].key = key;
    key_queue[key_head].state = state;
    key_head = next;
}

/* Laptop stand-ins for Vela's buttons: left back, down home, right menu.
 * Escape and Backspace are back too. */
static int map_button(SDL_Keycode k)
{
    switch(k) {
        case SDLK_LEFT:
        case SDLK_ESCAPE:
        case SDLK_BACKSPACE: return KEEL_BUTTON_BACK;
        case SDLK_DOWN: return KEEL_BUTTON_HOME;
        case SDLK_RIGHT: return KEEL_BUTTON_MENU;
        default: return -1;
    }
}

static uint32_t map_key(SDL_Keycode k, Uint16 mod)
{
    switch(k) {
        case SDLK_UP: return LV_KEY_UP;
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
        case SDLK_SPACE: return LV_KEY_ENTER;
        case SDLK_TAB: return (mod & KMOD_SHIFT) ? LV_KEY_PREV : LV_KEY_NEXT;
        case SDLK_HOME: return LV_KEY_HOME;
        case SDLK_END: return LV_KEY_END;
        default: return 0;
    }
}

static void events_cb(lv_timer_t * t)
{
    SDL_Event e;
    while(SDL_PollEvent(&e)) {
        switch(e.type) {
            case SDL_QUIT:
                exit(0);
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                if(e.button.button == SDL_BUTTON_LEFT) mouse_down = e.type == SDL_MOUSEBUTTONDOWN;
                mouse_point.x = e.button.x / scale;
                mouse_point.y = e.button.y / scale;
                break;
            case SDL_MOUSEMOTION:
                mouse_point.x = e.motion.x / scale;
                mouse_point.y = e.motion.y / scale;
                break;
            case SDL_KEYDOWN:
            case SDL_KEYUP: {
                /* Vela's three buttons are buttons, not keys: Keel sees each
                 * press and release and decides short or long itself. */
                int b = map_button(e.key.keysym.sym);
                if(b >= 0) {
                    if(!e.key.repeat) keel_button_event((keel_button_t)b, e.type == SDL_KEYDOWN);
                    break;
                }
                uint32_t k = map_key(e.key.keysym.sym, e.key.keysym.mod);
                if(k) key_push(k, e.type == SDL_KEYDOWN ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED);
                break;
            }
            case SDL_WINDOWEVENT:
                if(e.window.event == SDL_WINDOWEVENT_EXPOSED || e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                    lv_obj_invalidate(lv_screen_active());
                break;
            default:
                break;
        }
    }
}

static void mouse_read(lv_indev_t * indev, lv_indev_data_t * data)
{
    data->point = mouse_point;
    data->state = mouse_down ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

static void keypad_read(lv_indev_t * indev, lv_indev_data_t * data)
{
    if(key_tail == key_head) {
        data->key = last_key;
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }
    data->key = last_key = key_queue[key_tail].key;
    data->state = key_queue[key_tail].state;
    key_tail = (key_tail + 1) % KEY_QUEUE;
    data->continue_reading = key_tail != key_head;
}

static uint32_t tick_cb(void)
{
    return SDL_GetTicks();
}

lv_display_t * platform_sdl_init(int window_scale)
{
    scale = LV_CLAMP(1, window_scale, 4);
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) return NULL;
    lv_tick_set_cb(tick_cb);

    window = SDL_CreateWindow("keel", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, KEEL_HOR_RES * scale,
                              KEEL_VER_RES * scale, 0);
    if(!window) return NULL;

    lv_display_t * disp = lv_display_create(KEEL_HOR_RES, KEEL_VER_RES);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_XRGB8888);
    draw_buf = lv_draw_buf_create(KEEL_HOR_RES, KEEL_VER_RES, LV_COLOR_FORMAT_XRGB8888, LV_STRIDE_AUTO);
    lv_display_set_draw_buffers(disp, draw_buf, NULL);
    lv_display_set_render_mode(disp, LV_DISPLAY_RENDER_MODE_DIRECT);
    lv_display_set_flush_cb(disp, flush_cb);

    frame = SDL_CreateRGBSurfaceWithFormatFrom(draw_buf->data, KEEL_HOR_RES, KEEL_VER_RES, 32,
                                               (int)draw_buf->header.stride, SDL_PIXELFORMAT_XRGB8888);
    SDL_SetSurfaceBlendMode(frame, SDL_BLENDMODE_NONE);

    /* Mouse drag is one-finger touch. Keys feed a keypad that ui/ binds to
     * each screen's focus group. */
    lv_indev_t * mouse = lv_indev_create();
    lv_indev_set_type(mouse, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(mouse, mouse_read);
    lv_indev_set_display(mouse, disp);

    lv_indev_t * keys = lv_indev_create();
    lv_indev_set_type(keys, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(keys, keypad_read);
    lv_indev_set_display(keys, disp);

    lv_timer_create(events_cb, 5, NULL);
    return disp;
}
