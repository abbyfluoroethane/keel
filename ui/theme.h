/* Keel design tokens.
 *
 * Source: Abby's design system v1.1, with the Catppuccin Mocha surfaces
 * replaced by an AMOLED black theme. Every value lives in theme.c. This
 * header only declares them, plus the scale macro.
 */
#ifndef KEEL_THEME_H
#define KEEL_THEME_H

#include "lvgl.h"

/* The design system is specified in desktop pixels. The Vela panel is
 * about 233 ppi, so every size and space is multiplied by KEEL_SCALE.
 * Tune this on real hardware. */
#ifndef KEEL_SCALE
#define KEEL_SCALE 1.5f
#endif

/* Scale a design-system pixel value to panel pixels. */
#define KS(px) ((int32_t)((px) * KEEL_SCALE + 0.5f))

/* Panel size. Rendering is always this size, whatever the window zoom. */
#define KEEL_HOR_RES 480
#define KEEL_VER_RES 800

/* ---- Color ------------------------------------------------------------ */

typedef struct {
    lv_color_t base;
    lv_color_t raised;
    lv_color_t hover;
    lv_color_t line;
    lv_color_t text;
    lv_color_t muted;
    lv_color_t dim;
    lv_color_t accent;
    lv_color_t bar_start;
    lv_color_t focus;
    lv_color_t control_border;
    lv_color_t status_error;
    lv_color_t on_error;
    lv_color_t status_ok;
    lv_color_t on_ok;
} keel_colors_t;

extern const keel_colors_t keel_color;

/* ---- Space and shape -------------------------------------------------- */

typedef struct {
    int32_t space_1, space_2, space_3, space_4, space_6, space_8;
    int32_t margin;       /* page margin, left edge of everything */
    int32_t tile_gap;
    int32_t touch_min;    /* minimum touch target height */
    int32_t bar_edge;     /* energy bar height */
    int32_t focus_width;
    int32_t focus_offset;
    int32_t radius;
    int32_t status_bar_h; /* Keel addition, see theme.c */
    int32_t control_edge; /* 1px edge of sliders and toggles */
} keel_space_t;

extern const keel_space_t keel_space;

/* Cover art sizes. tools/gen_covers.py writes one PNG per size. */
#define KEEL_ART_FULL  480  /* now playing, full width */
#define KEEL_ART_LARGE 408  /* album screen: 480 - 2 * margin */
#define KEEL_ART_TILE  198  /* album grid: (408 - tile gap) / 2 */
#define KEEL_ART_THUMB 72   /* mini player: one touch target */

/* ---- Motion ----------------------------------------------------------- */

typedef struct {
    uint32_t duration;      /* ease-out, 200 to 300 ms */
    uint32_t stagger_step;  /* delay between list rows entering */
    uint32_t stagger_max;   /* rows after this many enter together */
    int32_t stagger_dist;   /* rows enter from this far to the right */
    uint32_t tap_flash;     /* how long the hover fill stays after a tap */
    uint32_t long_press;    /* hold this long for a long press */
    uint32_t idle_hide;     /* now playing hides large static text */
    uint32_t burn_shift;    /* status bar moves 1px this often */
    float parallax;         /* header travel / content travel */
} keel_motion_t;

extern const keel_motion_t keel_motion;

/* ---- Type ------------------------------------------------------------- */

typedef enum {
    KEEL_TYPE_HERO,
    KEEL_TYPE_TITLE,
    KEEL_TYPE_HEADLINE,
    KEEL_TYPE_HEADING,
    KEEL_TYPE_SUBHEADING,
    KEEL_TYPE_BODY,
    KEEL_TYPE_SMALL,
    KEEL_TYPE_LABEL,
    KEEL_TYPE_OVERLINE,
    KEEL_TYPE_DATA,
    KEEL_TYPE_NUMERAL,
    KEEL_TYPE_COUNT
} keel_type_t;

/* Load the fonts and build the styles. Returns false if a font is missing. */
bool keel_theme_init(const char * asset_dir);

/* Style for a type role: font, letter spacing, line height, text color. */
lv_style_t * keel_type_style(keel_type_t type);
const lv_font_t * keel_type_font(keel_type_t type);
/* The line height the design system asks for, in panel pixels. */
int32_t keel_type_line_height(keel_type_t type);

/* Create a label in a type role. Uppercase roles (headline, overline) get
 * an uppercased copy of `text`. Every other role shows `text` unchanged:
 * lowercase titles are written by hand in the data, never made in code. */
lv_obj_t * keel_text(lv_obj_t * parent, keel_type_t type, const char * text);
/* Change the text of a label made by keel_text, keeping its case rule. */
void keel_text_set(lv_obj_t * label, const char * text);

/* Write an uppercased copy of `src` into `dst`. ASCII and Latin-1 letters
 * are mapped; every other code point passes through. */
void keel_upper(char * dst, size_t dst_size, const char * src);

/* ---- Icons ------------------------------------------------------------
 * Phosphor (MIT), regular weight, from assets/icons/. The icon rule is
 * words first, Plex glyphs second, Phosphor third; Keel uses Phosphor only
 * where a word does not fit the job, such as status bar indicators. */
#define KEEL_ICON_WIFI_HIGH           "\xEE\x93\xAA" /* U+E4EA */
#define KEEL_ICON_WIFI_MEDIUM         "\xEE\x93\xAE" /* U+E4EE */
#define KEEL_ICON_WIFI_LOW            "\xEE\x93\xAC" /* U+E4EC */
#define KEEL_ICON_BLUETOOTH           "\xEE\x83\x9A" /* U+E0DA */
#define KEEL_ICON_BLUETOOTH_CONNECTED "\xEE\x83\x9C" /* U+E0DC */

/* A label showing one icon, sized to sit in a `data` line. */
lv_obj_t * keel_icon(lv_obj_t * parent, const char * icon);

/* ---- Shared styles ---------------------------------------------------- */

/* Black, borderless, square, no padding. Every screen starts from this. */
lv_style_t * keel_style_screen(void);
/* A bare container: transparent, square, no padding, no border. */
lv_style_t * keel_style_plain(void);
/* A tappable row: hover fill when pressed or focused, focus ring on keys. */
lv_style_t * keel_style_row(void);
lv_style_t * keel_style_row_pressed(void);
lv_style_t * keel_style_focus_ring(void);

#endif /* KEEL_THEME_H */
