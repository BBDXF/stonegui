/**
 * sg_theme.c — stonegui default theme
 *
 * A small, opinionated design system that makes the raw LVGL widgets look
 * closer to Flutter / Material 3 / modern web defaults. Implemented as a
 * custom LVGL theme whose parent is the built-in default theme, so we only
 * override what matters (colours, radii, spacing, elevation) and inherit the
 * rest.
 *
 * Layering (lowest → highest priority):
 *   1. default theme   — structural baseline
 *   2. sg_theme        — this file's overrides
 *   3. user styles     — per-widget style={…} props (local styles, always win)
 */

#include "sg_theme.h"
#include "src/themes/lv_theme_private.h"

/* ── Design tokens (light scheme) ───────────────────────────────────────── */

#define SG_PRIMARY        lv_color_hex(0x2563EB)  /* blue 600  — actions      */
#define SG_PRIMARY_DARK   lv_color_hex(0x1D4ED8)  /* pressed primary          */
#define SG_ON_PRIMARY     lv_color_hex(0xFFFFFF)  /* text/icon on primary     */
#define SG_SECONDARY      lv_color_hex(0x9333EA)  /* accent                   */
#define SG_BG             lv_color_hex(0xF4F5F7)  /* app / scaffold background */
#define SG_SURFACE        lv_color_hex(0xFFFFFF)  /* cards, fields            */
#define SG_ON_SURFACE     lv_color_hex(0x1F2937)  /* primary text (slate 800) */
#define SG_ON_VARIANT     lv_color_hex(0x6B7280)  /* secondary / placeholder  */
#define SG_OUTLINE        lv_color_hex(0xD1D5DB)  /* borders                  */
#define SG_TRACK          lv_color_hex(0xE5E7EB)  /* switch/bar background     */

#define SG_RADIUS_BTN     10
#define SG_RADIUS_FIELD   10
#define SG_PAD_FIELD      12
#define SG_PAD_BTN_HOR    20
#define SG_PAD_BTN_VER    10

/* ── Shared styles (persistent — referenced by widgets for their lifetime) ── */

static bool      g_styles_inited = false;
static lv_theme_t g_theme;        /* our theme (parent = default theme)        */

static lv_style_t st_screen;      /* root background + inherited text colour    */
static lv_style_t st_btn, st_btn_pressed;
static lv_style_t st_field, st_field_focused, st_field_placeholder;
static lv_style_t st_sw_bg, st_sw_bg_on, st_sw_knob;
static lv_style_t st_bar_main, st_bar_indic;

static void styles_init_once(void) {
    if (g_styles_inited) return;
    g_styles_inited = true;

    /* Screen: light scaffold background + app-wide default text colour.
     * text_color is inheritable, so every label inherits SG_ON_SURFACE unless
     * an ancestor (e.g. a Button) or the widget itself overrides it. */
    lv_style_init(&st_screen);
    lv_style_set_bg_color(&st_screen, SG_BG);
    lv_style_set_bg_opa(&st_screen, LV_OPA_COVER);
    lv_style_set_text_color(&st_screen, SG_ON_SURFACE);

    /* Button: filled primary, rounded, soft elevation, white label. */
    lv_style_init(&st_btn);
    lv_style_set_bg_color(&st_btn, SG_PRIMARY);
    lv_style_set_bg_opa(&st_btn, LV_OPA_COVER);
    lv_style_set_text_color(&st_btn, SG_ON_PRIMARY);   /* inherited by label   */
    lv_style_set_border_width(&st_btn, 0);
    lv_style_set_radius(&st_btn, SG_RADIUS_BTN);
    lv_style_set_pad_hor(&st_btn, SG_PAD_BTN_HOR);
    lv_style_set_pad_ver(&st_btn, SG_PAD_BTN_VER);
    lv_style_set_shadow_color(&st_btn, lv_color_black());
    lv_style_set_shadow_opa(&st_btn, LV_OPA_20);
    lv_style_set_shadow_width(&st_btn, 10);
    lv_style_set_shadow_offset_y(&st_btn, 4);

    /* Pressed: darken + flatten the shadow; cancel the default theme's grow. */
    lv_style_init(&st_btn_pressed);
    lv_style_set_bg_color(&st_btn_pressed, SG_PRIMARY_DARK);
    lv_style_set_shadow_opa(&st_btn_pressed, LV_OPA_10);
    lv_style_set_shadow_width(&st_btn_pressed, 4);
    lv_style_set_shadow_offset_y(&st_btn_pressed, 1);
    lv_style_set_transform_width(&st_btn_pressed, 0);
    lv_style_set_transform_height(&st_btn_pressed, 0);

    /* Text field (Input): outlined, rounded, comfortable padding. */
    lv_style_init(&st_field);
    lv_style_set_bg_color(&st_field, SG_SURFACE);
    lv_style_set_bg_opa(&st_field, LV_OPA_COVER);
    lv_style_set_text_color(&st_field, SG_ON_SURFACE);
    lv_style_set_radius(&st_field, SG_RADIUS_FIELD);
    lv_style_set_border_color(&st_field, SG_OUTLINE);
    lv_style_set_border_width(&st_field, 1);
    lv_style_set_pad_all(&st_field, SG_PAD_FIELD);

    /* Focused field: primary 2px outline (Material focus ring). */
    lv_style_init(&st_field_focused);
    lv_style_set_border_color(&st_field_focused, SG_PRIMARY);
    lv_style_set_border_width(&st_field_focused, 2);

    /* Placeholder text colour. */
    lv_style_init(&st_field_placeholder);
    lv_style_set_text_color(&st_field_placeholder, SG_ON_VARIANT);

    /* Switch: pill track (grey off / primary on) with a white circular knob. */
    lv_style_init(&st_sw_bg);
    lv_style_set_bg_color(&st_sw_bg, SG_TRACK);
    lv_style_set_bg_opa(&st_sw_bg, LV_OPA_COVER);
    lv_style_set_radius(&st_sw_bg, LV_RADIUS_CIRCLE);

    lv_style_init(&st_sw_bg_on);
    lv_style_set_bg_color(&st_sw_bg_on, SG_PRIMARY);

    lv_style_init(&st_sw_knob);
    lv_style_set_bg_color(&st_sw_knob, lv_color_white());
    lv_style_set_radius(&st_sw_knob, LV_RADIUS_CIRCLE);

    /* Progress (bar): thin pill, light track, primary indicator. */
    lv_style_init(&st_bar_main);
    lv_style_set_bg_color(&st_bar_main, SG_TRACK);
    lv_style_set_bg_opa(&st_bar_main, LV_OPA_COVER);
    lv_style_set_radius(&st_bar_main, LV_RADIUS_CIRCLE);
    lv_style_set_border_width(&st_bar_main, 0);

    lv_style_init(&st_bar_indic);
    lv_style_set_bg_color(&st_bar_indic, SG_PRIMARY);
    lv_style_set_bg_opa(&st_bar_indic, LV_OPA_COVER);
    lv_style_set_radius(&st_bar_indic, LV_RADIUS_CIRCLE);
}

/* ── apply callback: layer our styles by widget class ───────────────────── */

static void sg_theme_apply_cb(lv_theme_t *th, lv_obj_t *obj) {
    (void)th;

    /* Screens (no parent) get the scaffold background + default text colour. */
    if (lv_obj_get_parent(obj) == NULL) {
        lv_obj_add_style(obj, &st_screen, 0);
        return;
    }

    const lv_obj_class_t *cls = lv_obj_get_class(obj);

    if (cls == &lv_button_class) {
        lv_obj_add_style(obj, &st_btn, 0);
        lv_obj_add_style(obj, &st_btn_pressed, LV_STATE_PRESSED);
    }
    else if (cls == &lv_textarea_class) {
        lv_obj_add_style(obj, &st_field, 0);
        lv_obj_add_style(obj, &st_field_focused, LV_STATE_FOCUSED);
        lv_obj_add_style(obj, &st_field_placeholder,
                         LV_PART_TEXTAREA_PLACEHOLDER);
    }
    else if (cls == &lv_switch_class) {
        lv_obj_add_style(obj, &st_sw_bg, 0);
        lv_obj_add_style(obj, &st_sw_bg_on, LV_STATE_CHECKED);
        lv_obj_add_style(obj, &st_sw_knob, LV_PART_KNOB);
    }
    else if (cls == &lv_bar_class) {
        lv_obj_add_style(obj, &st_bar_main, 0);
        lv_obj_add_style(obj, &st_bar_indic, LV_PART_INDICATOR);
    }
    /* Labels/Text intentionally have no style here: they inherit text_color
     * from their nearest ancestor (screen → dark, Button → white). */
}

/* ── public API ─────────────────────────────────────────────────────────── */

void sg_theme_init(lv_display_t *disp, const lv_font_t *font) {
    styles_init_once();

    lv_display_t *d = disp ? disp : lv_display_get_default();

    /* (Re)initialise the built-in default theme — handles base structure and
     * carries the default font/colours. */
    lv_theme_t *base = lv_theme_default_init(
        d, SG_PRIMARY, SG_SECONDARY,
        /* dark = */ false,
        font ? font : LV_FONT_DEFAULT);

    /* Copy fonts/colours from the base so runtime lookups on the active theme
     * (e.g. lv_theme_get_font_normal) resolve correctly, then chain + override. */
    g_theme = *base;
    g_theme.parent   = base;
    g_theme.apply_cb = sg_theme_apply_cb;

    lv_display_set_theme(d, &g_theme);
}

void sg_theme_set_font(lv_display_t *disp, const lv_font_t *font) {
    sg_theme_init(disp, font);
    /* Restyle any widgets already created so they pick up the new font. */
    lv_obj_report_style_change(NULL);
}
