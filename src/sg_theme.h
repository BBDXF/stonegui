#ifndef SG_THEME_H
#define SG_THEME_H

#include "lvgl.h"

/**
 * sg_theme — stonegui's default look & feel.
 *
 * Provides a modern, Flutter/Material-inspired baseline for every widget by
 * layering a custom LVGL theme on top of the built-in default theme:
 *
 *     default theme (base)  →  sg_theme (overrides)  →  user inline styles
 *
 * The base theme handles structural defaults; sg_theme restyles colours,
 * corner radii, spacing and elevation so widgets feel consistent out of the
 * box. Per-widget `style={…}` props from JS always win on top.
 */

/* Install the theme on `disp` (NULL = default display). `font` is the default
 * text font (NULL keeps LVGL's built-in font). Call once at startup. */
void sg_theme_init(lv_display_t *disp, const lv_font_t *font);

/* Swap the default text font, keeping all sg_theme customizations.
 * Intended to be called before the UI tree is built (see setDefaultFont). */
void sg_theme_set_font(lv_display_t *disp, const lv_font_t *font);

#endif /* SG_THEME_H */
