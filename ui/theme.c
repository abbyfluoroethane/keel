/* Keel design tokens. Every color, space, size and duration is here. */
#include "theme.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

/* ---- Color: AMOLED theme ---------------------------------------------- */

const keel_colors_t keel_color = {
    .base           = LV_COLOR_MAKE(0x00, 0x00, 0x00), /* true black, every screen */
    .raised         = LV_COLOR_MAKE(0x11, 0x11, 0x1b), /* Mocha crust */
    .hover          = LV_COLOR_MAKE(0x1e, 0x1e, 0x2e), /* Mocha base */
    .line           = LV_COLOR_MAKE(0x31, 0x32, 0x44), /* Mocha surface0, dividers only */
    .text           = LV_COLOR_MAKE(0xcd, 0xd6, 0xf4), /* Mocha text */
    .muted          = LV_COLOR_MAKE(0xa6, 0xad, 0xc8), /* Mocha subtext0, on base only */
    .dim            = LV_COLOR_MAKE(0x58, 0x5b, 0x70), /* Mocha surface2, decorative text */
    .accent         = LV_COLOR_MAKE(0xf5, 0xc2, 0xe7), /* Mocha pink, decoration only */
    .bar_start      = LV_COLOR_MAKE(0xfa, 0xb3, 0x87), /* Mocha peach, energy bar start */
    .focus          = LV_COLOR_MAKE(0xf5, 0xc2, 0xe7),
    .control_border = LV_COLOR_MAKE(0x7f, 0x84, 0x9c),
    .status_error   = LV_COLOR_MAKE(0xf3, 0x8b, 0xa8),
    .on_error       = LV_COLOR_MAKE(0x11, 0x11, 0x1b),
    .status_ok      = LV_COLOR_MAKE(0xa6, 0xe3, 0xa1),
    .on_ok          = LV_COLOR_MAKE(0x11, 0x11, 0x1b),
};

/* ---- Space and shape -------------------------------------------------- */

const keel_space_t keel_space = {
    .space_1 = KS(4),
    .space_2 = KS(8),
    .space_3 = KS(12),
    .space_4 = KS(16),
    .space_6 = KS(24),
    .space_8 = KS(32),
    .margin = KS(24),
    .tile_gap = KS(8),
    .touch_min = KS(48),
    .bar_edge = KS(6),
    .focus_width = KS(2),
    .focus_offset = KS(2),
    .radius = 0,
    /* New for Keel: the design system has no status bar. One `data` line
     * (30px) plus space-1 above, so the clock clears the panel edge. */
    .status_bar_h = KS(24),
    .control_edge = KS(1),
};

/* ---- Motion ----------------------------------------------------------- */

/* The design system gives only "ease-out, 200 to 300 ms". The rest are
 * new for Keel, each with the reason. */
const keel_motion_t keel_motion = {
    .duration = 250,
    /* Stagger: 30 ms per row keeps a full screen of rows (about 10)
     * inside the 300 ms budget plus one duration. */
    .stagger_step = 30,
    .stagger_max = 10,
    /* Rows enter from space-8 to the right: far enough to read as motion,
     * short enough not to look like a page change. */
    .stagger_dist = KS(32),
    /* Tap feedback: the hover fill fades out over this after release. */
    .tap_flash = 150,
    /* Long press, for the menu button and for item menus. LVGL's touch
     * default, so a held button and a held row feel the same. */
    .long_press = 400,
    /* Burn-in rules from the design system: 30 s idle, "every few minutes". */
    .idle_hide = 30000,
    .burn_shift = 180000,
    /* Header travel per pixel of content travel. Under 1, so the header
     * moves slower than the content. Computed per pane in the pivot from
     * real header widths; this is the floor. */
    .parallax = 0.6f,
};

/* ---- Type ------------------------------------------------------------- */

typedef struct {
    const char * file;       /* in assets/fonts */
    int32_t size;            /* design-system px, before KEEL_SCALE */
    int32_t line;            /* design-system px, before KEEL_SCALE */
    float tracking_em;       /* letter spacing in em */
    bool upper;              /* uppercase at render time */
    bool fallback;           /* chain Plex Sans Thai + Arabic for track names */
} type_spec_t;

/* Sizes are the design system's desktop sizes. KS() scales them. The
 * results match the Keel spec table (title 84/90, ...) except hero.
 * Hero is 120px, not the table's 144: hero is only used for pivot headers,
 * and at 144 "albums" alone is 453px, so the next pane's header could never
 * peek in at the right edge of a 480px panel. Its 108 line asks for less
 * than its own ascenders need, so its box comes out at 121 (see below). */
static const type_spec_t type_spec[KEEL_TYPE_COUNT] = {
    [KEEL_TYPE_HERO]       = {"IBMPlexSans-Light.ttf",    80, 72, -0.03f, false, false},
    [KEEL_TYPE_TITLE]      = {"IBMPlexSans-Light.ttf",    56, 60, -0.02f, false, false},
    [KEEL_TYPE_HEADLINE]   = {"IBMPlexSans-Bold.ttf",     40, 36, -0.03f, true,  false},
    [KEEL_TYPE_HEADING]    = {"IBMPlexSans-SemiBold.ttf", 24, 32, -0.01f, false, true},
    [KEEL_TYPE_SUBHEADING] = {"IBMPlexSans-SemiBold.ttf", 18, 24,  0.00f, false, true},
    [KEEL_TYPE_BODY]       = {"IBMPlexSans-Regular.ttf",  16, 24,  0.00f, false, true},
    [KEEL_TYPE_SMALL]      = {"IBMPlexSans-Regular.ttf",  14, 20,  0.00f, false, true},
    [KEEL_TYPE_LABEL]      = {"IBMPlexSans-SemiBold.ttf", 12, 16,  0.00f, false, true},
    [KEEL_TYPE_OVERLINE]   = {"IBMPlexMono-Medium.ttf",   12, 16,  0.08f, true,  false},
    [KEEL_TYPE_DATA]       = {"IBMPlexMono-Medium.ttf",   14, 20,  0.00f, false, false},
    /* Numeral: the spec allows 72 to 144. Keel uses the top of the range,
     * since the one place it appears (radio) has nothing else loud. */
    [KEEL_TYPE_NUMERAL]    = {"IBMPlexSans-Bold.ttf",     96, 96, -0.04f, false, false},
};

/* Glyph fallbacks for real-world titles (e.g. Ninajirachi's "ฅ^•ﻌ•^ฅ").
 * Same family and license, matched by weight. */
static const char * fallback_file(const char * file, int which)
{
    bool semibold = strstr(file, "SemiBold") != NULL;
    if(which == 0) return semibold ? "IBMPlexSansThai-SemiBold.ttf" : "IBMPlexSansThai-Regular.ttf";
    return semibold ? "IBMPlexSansArabic-SemiBold.ttf" : "IBMPlexSansArabic-Regular.ttf";
}

static lv_font_t * fonts[KEEL_TYPE_COUNT];
static lv_font_t * icon_font;
static lv_style_t style_icon;
static lv_style_t type_styles[KEEL_TYPE_COUNT];
static int32_t line_heights[KEEL_TYPE_COUNT];

static lv_style_t style_screen, style_plain, style_row, style_row_pressed, style_focus;
static lv_style_transition_dsc_t row_trans, row_trans_instant;
static const lv_style_prop_t row_trans_props[] = {LV_STYLE_BG_OPA, 0};

/* How far the tallest (`up`) or deepest glyph of `chars` reaches from the
 * baseline, in px. */
static int32_t glyph_extent(const lv_font_t * font, const char * chars, bool up)
{
    int32_t most = 0;
    for(const char * c = chars; *c; c++) {
        lv_font_glyph_dsc_t g;
        if(!lv_font_get_glyph_dsc(font, &g, (uint32_t)*c, 0)) continue;
        int32_t reach = up ? g.ofs_y + (int32_t)g.box_h : -g.ofs_y;
        if(reach > most) most = reach;
    }
    return most;
}

static lv_font_t * load_font_from(const char * dir, const char * sub, const char * file, int32_t px)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/%s/%s", dir, sub, file);
    lv_font_t * f = lv_freetype_font_create(path, LV_FREETYPE_FONT_RENDER_MODE_BITMAP, (uint32_t)px,
                                            LV_FREETYPE_FONT_STYLE_NORMAL);
    if(!f) LV_LOG_ERROR("keel: cannot load font %s", path);
    return f;
}

static lv_font_t * load_font(const char * dir, const char * file, int32_t px)
{
    return load_font_from(dir, "fonts", file, px);
}

bool keel_theme_init(const char * asset_dir)
{
    /* lv_init() has already started FreeType (LV_USE_FREETYPE). */
    for(int t = 0; t < KEEL_TYPE_COUNT; t++) {
        const type_spec_t * s = &type_spec[t];
        int32_t px = KS(s->size);
        fonts[t] = load_font(asset_dir, s->file, px);
        if(!fonts[t]) return false;
        if(s->fallback) {
            lv_font_t * thai = load_font(asset_dir, fallback_file(s->file, 0), px);
            lv_font_t * arabic = load_font(asset_dir, fallback_file(s->file, 1), px);
            if(thai && arabic) {
                fonts[t]->fallback = thai;
                thai->fallback = arabic;
            }
        }

        line_heights[t] = KS(s->line);
        int32_t natural = lv_font_get_line_height(fonts[t]);
        /* CSS-style line box: split the difference between the line height
         * and the font's own height evenly above and below. */
        int32_t lead = line_heights[t] - natural;
        int32_t lead_top = lead >= 0 ? lead / 2 : -((-lead) / 2);
        int32_t lead_bottom = lead - lead_top;
        if(lead < 0) {
            /* A negative lead may only trim the font's empty space, never
             * the glyphs: LVGL clips a label to its own box, so trimming
             * into ascenders cuts the tops off l and the dot off i. Spill
             * what one side cannot take onto the other; what neither can
             * take makes the box taller than the spec asks (only hero). */
            int32_t top_slack = LV_MAX(0, natural - fonts[t]->base_line - glyph_extent(fonts[t], "bdfhiklt", true));
            int32_t bottom_slack = LV_MAX(0, fonts[t]->base_line - glyph_extent(fonts[t], "gjpqy", false));
            lead_top = LV_MAX(lead_top, -top_slack);
            lead_bottom = LV_MAX(lead_bottom, -bottom_slack);
            lead_top = LV_MAX(lead_top + (lead - lead_top - lead_bottom), -top_slack);
            lead_bottom = LV_MAX(lead - lead_top, -bottom_slack);
        }

        lv_style_t * st = &type_styles[t];
        lv_style_init(st);
        lv_style_set_text_font(st, fonts[t]);
        lv_style_set_text_color(st, keel_color.text);
        lv_style_set_text_letter_space(st, (int32_t)lroundf(s->tracking_em * (float)px));
        lv_style_set_text_line_space(st, lead);
        lv_style_set_pad_top(st, lead_top);
        lv_style_set_pad_bottom(st, lead_bottom);
        lv_style_set_pad_left(st, 0);
        lv_style_set_pad_right(st, 0);
    }

    /* New for Keel: icon size. 16 design-px (24 on the panel): Phosphor
     * draws inside its em with some air, so at the 14px `data` size the
     * icons read smaller than the digits beside them. */
    icon_font = load_font_from(asset_dir, "icons", "Phosphor-Regular.ttf", KS(16));
    if(!icon_font) return false;
    lv_style_init(&style_icon);
    lv_style_set_text_font(&style_icon, icon_font);
    lv_style_set_text_color(&style_icon, keel_color.text);

    lv_style_init(&style_plain);
    lv_style_set_bg_opa(&style_plain, LV_OPA_TRANSP);
    lv_style_set_border_width(&style_plain, 0);
    lv_style_set_outline_width(&style_plain, 0);
    lv_style_set_shadow_width(&style_plain, 0);
    lv_style_set_radius(&style_plain, keel_space.radius);
    lv_style_set_pad_all(&style_plain, 0);
    lv_style_set_pad_gap(&style_plain, 0);
    lv_style_set_text_color(&style_plain, keel_color.text);
    lv_style_set_text_font(&style_plain, fonts[KEEL_TYPE_BODY]);

    lv_style_init(&style_screen);
    lv_style_set_bg_color(&style_screen, keel_color.base);
    lv_style_set_bg_opa(&style_screen, LV_OPA_COVER);
    lv_style_set_border_width(&style_screen, 0);
    lv_style_set_radius(&style_screen, 0);
    lv_style_set_pad_all(&style_screen, 0);
    lv_style_set_text_color(&style_screen, keel_color.text);
    lv_style_set_text_font(&style_screen, fonts[KEEL_TYPE_BODY]);

    /* Rows: no box, no divider. Hover fill only while pressed or focused,
     * fading out after release so a quick tap still shows. */
    lv_style_transition_dsc_init(&row_trans, row_trans_props, lv_anim_path_ease_out,
                                 keel_motion.tap_flash, 0, NULL);
    lv_style_init(&style_row);
    lv_style_set_bg_color(&style_row, keel_color.hover);
    lv_style_set_bg_opa(&style_row, LV_OPA_TRANSP);
    lv_style_set_transition(&style_row, &row_trans);

    lv_style_init(&style_row_pressed);
    lv_style_set_bg_opa(&style_row_pressed, LV_OPA_COVER);
    /* Fill on instantly. LVGL needs a real descriptor here, not NULL. */
    lv_style_transition_dsc_init(&row_trans_instant, row_trans_props, lv_anim_path_linear, 0, 0, NULL);
    lv_style_set_transition(&style_row_pressed, &row_trans_instant);

    lv_style_init(&style_focus);
    lv_style_set_bg_opa(&style_focus, LV_OPA_COVER);
    lv_style_set_outline_color(&style_focus, keel_color.focus);
    lv_style_set_outline_width(&style_focus, keel_space.focus_width);
    lv_style_set_outline_pad(&style_focus, keel_space.focus_offset);
    lv_style_set_outline_opa(&style_focus, LV_OPA_COVER);

    return true;
}

lv_style_t * keel_type_style(keel_type_t type) { return &type_styles[type]; }
const lv_font_t * keel_type_font(keel_type_t type) { return fonts[type]; }
int32_t keel_type_line_height(keel_type_t type) { return line_heights[type]; }
lv_style_t * keel_style_screen(void) { return &style_screen; }
lv_style_t * keel_style_plain(void) { return &style_plain; }
lv_style_t * keel_style_row(void) { return &style_row; }
lv_style_t * keel_style_row_pressed(void) { return &style_row_pressed; }
lv_style_t * keel_style_focus_ring(void) { return &style_focus; }

/* ---- Labels ----------------------------------------------------------- */

/* A negative line box (pad < 0) would clip ascenders and descenders to the
 * label's own area. Widen the draw area by the overhang. */
static void ext_draw_cb(lv_event_t * e)
{
    lv_obj_t * obj = lv_event_get_target_obj(e);
    int32_t top = lv_obj_get_style_pad_top(obj, LV_PART_MAIN);
    int32_t bottom = lv_obj_get_style_pad_bottom(obj, LV_PART_MAIN);
    int32_t over = LV_MAX(-top, -bottom);
    if(over > 0) lv_event_set_ext_draw_size(e, over);
}

void keel_upper(char * dst, size_t dst_size, const char * src)
{
    size_t o = 0;
    for(const unsigned char * p = (const unsigned char *)src; *p && o + 2 < dst_size; p++) {
        unsigned char c = *p;
        if(c >= 'a' && c <= 'z') {
            dst[o++] = (char)(c - 32);
        }
        else if(c == 0xC3 && p[1] >= 0xA0 && p[1] <= 0xBE && p[1] != 0xB7) {
            /* Latin-1 lowercase (à..þ, not ÷) is two bytes: C3 A0..BE. */
            dst[o++] = (char)c;
            dst[o++] = (char)(p[1] - 0x20);
            p++;
        }
        else {
            dst[o++] = (char)c;
        }
    }
    dst[o] = '\0';
}

void keel_text_set(lv_obj_t * label, const char * text)
{
    keel_type_t type = (keel_type_t)(intptr_t)lv_obj_get_user_data(label);
    if(type_spec[type].upper) {
        char buf[256];
        keel_upper(buf, sizeof(buf), text);
        lv_label_set_text(label, buf);
    }
    else {
        lv_label_set_text(label, text);
    }
}

lv_obj_t * keel_text(lv_obj_t * parent, keel_type_t type, const char * text)
{
    lv_obj_t * l = lv_label_create(parent);
    lv_obj_set_clickable(l, false);
    lv_obj_add_style(l, &type_styles[type], 0);
    lv_obj_set_user_data(l, (void *)(intptr_t)type);
    lv_obj_add_event_cb(l, ext_draw_cb, LV_EVENT_REFR_EXT_DRAW_SIZE, NULL);
    /* The style was added before the callback existed; ask again. */
    lv_obj_refresh_ext_draw_size(l);
    keel_text_set(l, text);
    return l;
}

lv_obj_t * keel_icon(lv_obj_t * parent, const char * icon)
{
    lv_obj_t * l = lv_label_create(parent);
    lv_obj_set_clickable(l, false);
    lv_obj_add_style(l, &style_icon, 0);
    lv_label_set_text(l, icon);
    return l;
}
