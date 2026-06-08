/**
 * lv_bindings.c
 *
 * QuickJS native module "lvgl" — exposes LVGL widget operations to JS.
 *
 * JS API (HostRenderer):
 *   createNode(type)            → opaque node handle (int ptr as JS int)
 *   appendChild(parent, child)
 *   removeChild(parent, child)
 *   setProperty(node, key, val)
 *   addEvent(node, event, cb)   → event name strings: "click","longpress"
 *   dispose(node)
 *   getScreen()                 → root screen node
 */

#include "lv_bindings.h"
#include "sg_theme.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "lvgl.h"
#include "quickjs-libc.h"
#include "src/libs/tiny_ttf/lv_tiny_ttf.h"
#include "src/drivers/sdl/lv_sdl_window.h"

#include <SDL.h>

/* ── input group (keyboard focus) ───────────────────────────────────────── */

static lv_group_t *g_group = NULL;

void lv_bindings_set_group(lv_group_t *group) {
    g_group = group;
}

/* ── IME candidate-window positioning ───────────────────────────────────────
 *
 * Tell SDL where the focused text field is so the OS IME places its candidate
 * (composition) window next to the caret instead of at the screen origin.
 * LVGL screen coordinates are scaled by the SDL window zoom to get pixels.
 */
static void sg_update_ime_rect(lv_obj_t *ta) {
    lv_area_t a;
    lv_obj_get_coords(ta, &a);

    uint8_t zoom = 1;
    lv_display_t *disp = lv_display_get_default();
    if (disp) {
        uint8_t z = lv_sdl_window_get_zoom(disp);
        if (z) zoom = z;
    }

    SDL_Rect rect;
    rect.x = a.x1 * zoom;
    rect.y = a.y1 * zoom;
    rect.w = (a.x2 - a.x1 + 1) * zoom;
    rect.h = (a.y2 - a.y1 + 1) * zoom;
    SDL_SetTextInputRect(&rect);
}

static void sg_ime_event_cb(lv_event_t *e) {
    lv_obj_t *ta = lv_event_get_target(e);
    sg_update_ime_rect(ta);
}

/* ── helpers ────────────────────────────────────────────────────────────── */

static lv_obj_t *js_to_obj(JSContext *ctx, JSValueConst v) {
    int64_t ptr;
    JS_ToInt64(ctx, &ptr, v);
    return (lv_obj_t *)(uintptr_t)ptr;
}

static JSValue obj_to_js(JSContext *ctx, lv_obj_t *obj) {
    return JS_NewInt64(ctx, (int64_t)(uintptr_t)obj);
}

/* colour string → { lv_color_t, lv_opa_t }
 *   "#rrggbb"       — opaque
 *   "#rrggbbaa"     — with alpha (00=transparent, ff=opaque)
 *   named colours   — black white red green blue yellow cyan magenta
 *                     orange purple pink gray/grey silver lime maroon
 *                     navy olive teal transparent
 */
typedef struct { lv_color_t color; lv_opa_t opa; } sg_color_t;

static sg_color_t parse_color_ex(const char *s) {
    sg_color_t out = { lv_color_black(), LV_OPA_COVER };
    if (!s) return out;

    if (s[0] == '#') {
        unsigned int r = 0, g = 0, b = 0, a = 255;
        size_t len = strlen(s + 1);
        if (len >= 8)
            sscanf(s + 1, "%02x%02x%02x%02x", &r, &g, &b, &a);
        else
            sscanf(s + 1, "%02x%02x%02x", &r, &g, &b);
        out.color = lv_color_make(r, g, b);
        out.opa   = (lv_opa_t)a;
        return out;
    }

    static const struct { const char *name; uint8_t r, g, b; lv_opa_t opa; } table[] = {
        { "transparent", 0,   0,   0,   LV_OPA_TRANSP },
        { "black",       0,   0,   0,   LV_OPA_COVER  },
        { "white",       255, 255, 255, LV_OPA_COVER  },
        { "red",         255, 0,   0,   LV_OPA_COVER  },
        { "green",       0,   128, 0,   LV_OPA_COVER  },
        { "blue",        0,   0,   255, LV_OPA_COVER  },
        { "yellow",      255, 255, 0,   LV_OPA_COVER  },
        { "cyan",        0,   255, 255, LV_OPA_COVER  },
        { "magenta",     255, 0,   255, LV_OPA_COVER  },
        { "orange",      255, 165, 0,   LV_OPA_COVER  },
        { "purple",      128, 0,   128, LV_OPA_COVER  },
        { "pink",        255, 192, 203, LV_OPA_COVER  },
        { "gray",        128, 128, 128, LV_OPA_COVER  },
        { "grey",        128, 128, 128, LV_OPA_COVER  },
        { "silver",      192, 192, 192, LV_OPA_COVER  },
        { "lime",        0,   255, 0,   LV_OPA_COVER  },
        { "maroon",      128, 0,   0,   LV_OPA_COVER  },
        { "navy",        0,   0,   128, LV_OPA_COVER  },
        { "olive",       128, 128, 0,   LV_OPA_COVER  },
        { "teal",        0,   128, 128, LV_OPA_COVER  },
    };
    for (size_t i = 0; i < sizeof(table) / sizeof(table[0]); i++) {
        if (strcmp(s, table[i].name) == 0) {
            out.color = lv_color_make(table[i].r, table[i].g, table[i].b);
            out.opa   = table[i].opa;
            return out;
        }
    }
    return out;
}

/* Convenience wrapper — colour only (alpha ignored) */
static lv_color_t parse_color(const char *s) {
    return parse_color_ex(s).color;
}

/* size value: number → px; string "NN%" → lv_pct(NN); "fill"/"100%" → 100% */
static int32_t parse_size(JSContext *ctx, JSValueConst v) {
    if (JS_IsString(v)) {
        const char *s = JS_ToCString(ctx, v);
        int32_t out = 0;
        if (s) {
            if (strcmp(s, "fill") == 0) {
                out = lv_pct(100);
            } else {
                int n = 0;
                if (sscanf(s, "%d", &n) == 1)
                    out = (strchr(s, '%') != NULL) ? lv_pct(n) : n;
            }
        }
        JS_FreeCString(ctx, s);
        return out;
    }
    int32_t n; JS_ToInt32(ctx, &n, v);
    return n;
}

/* ── event callback bridge ──────────────────────────────────────────────── */

typedef struct {
    JSContext *ctx;
    JSValue    fn;
} EventCb;

/* Read a widget's "current value" as a JS value, so event handlers can be
 * called as onChange(value) in the React/Vue style:
 *   Switch / Checkbox      → bool   (checked)
 *   Slider / Bar / Arc     → number (value)
 *   Dropdown / Roller      → number (selected index)
 *   Input (textarea)       → string (text)
 *   otherwise              → undefined
 */
static JSValue widget_value_to_js(JSContext *ctx, lv_obj_t *obj) {
    if (!obj) return JS_UNDEFINED;
    const lv_obj_class_t *cls = lv_obj_get_class(obj);

    if (cls == &lv_switch_class || cls == &lv_checkbox_class)
        return JS_NewBool(ctx, lv_obj_has_state(obj, LV_STATE_CHECKED));
    if (cls == &lv_slider_class)
        return JS_NewInt32(ctx, lv_slider_get_value(obj));
    if (cls == &lv_bar_class)
        return JS_NewInt32(ctx, lv_bar_get_value(obj));
    if (cls == &lv_arc_class)
        return JS_NewInt32(ctx, lv_arc_get_value(obj));
    if (cls == &lv_dropdown_class)
        return JS_NewInt32(ctx, (int32_t)lv_dropdown_get_selected(obj));
    if (cls == &lv_roller_class)
        return JS_NewInt32(ctx, (int32_t)lv_roller_get_selected(obj));
    if (cls == &lv_textarea_class)
        return JS_NewString(ctx, lv_textarea_get_text(obj));

    return JS_UNDEFINED;
}

static void lv_event_dispatch(lv_event_t *e) {
    EventCb *ecb = (EventCb *)lv_event_get_user_data(e);
    if (!ecb) return;
    lv_obj_t *target = lv_event_get_current_target_obj(e);
    JSValue arg = widget_value_to_js(ecb->ctx, target);
    JSValue ret = JS_Call(ecb->ctx, ecb->fn, JS_UNDEFINED, 1, &arg);
    if (JS_IsException(ret))
        js_std_dump_error(ecb->ctx);
    JS_FreeValue(ecb->ctx, ret);
    JS_FreeValue(ecb->ctx, arg);
}

/* Released when the owning widget is deleted — frees the JS handler + struct */
static void lv_event_free(lv_event_t *e) {
    EventCb *ecb = (EventCb *)lv_event_get_user_data(e);
    if (!ecb) return;
    JS_FreeValue(ecb->ctx, ecb->fn);
    lv_free(ecb);
}

/* Strip the default theme's decorations from a generic container so a "View"
 * behaves like a React-Native View: a transparent, borderless, zero-padding
 * layout box. User styles (backgroundColor, borderRadius, padding…) layer on
 * top of this clean slate. */

/* Extra drawing margin (px) a layout container reserves around itself so child
 * decorations that spill past the box (e.g. a slider/arc knob at the track's
 * ends) are not clipped. LV_OBJ_FLAG_OVERFLOW_VISIBLE only lifts the clip up to
 * the parent's own ext_draw_size, so a plain (ext_draw_size == 0) container
 * would still clip — we must contribute a non-zero size here. */
#define SG_CONTAINER_EXT_DRAW 16

static void sg_container_ext_draw_cb(lv_event_t *e) {
    /* lv_event_set_ext_draw_size() already keeps the running maximum. */
    lv_event_set_ext_draw_size(e, SG_CONTAINER_EXT_DRAW);
}

static void make_clean_container(lv_obj_t *obj) {
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_outline_width(obj, 0, 0);
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_style_pad_row(obj, 0, 0);
    lv_obj_set_style_pad_column(obj, 0, 0);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    /* Behave like CSS `overflow: visible` (the default for flex boxes): let
     * child decorations that extend past the box draw outside it instead of
     * being clipped. Without this, a slider/arc knob (which is larger than the
     * track and sits at the track's ends/edges) gets cut off at the start
     * position and along the top/bottom.
     *
     * The flag alone is not enough: LVGL only widens a child's clip area by the
     * *parent's* ext_draw_size, which is 0 for a plain transparent container.
     * So we also reserve a small ext_draw_size via the event below. */
    lv_obj_add_flag(obj, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_add_event_cb(obj, sg_container_ext_draw_cb,
                        LV_EVENT_REFR_EXT_DRAW_SIZE, NULL);
    lv_obj_refresh_ext_draw_size(obj);
}

/* ── module functions ───────────────────────────────────────────────────── */

/* Fixed spinner arc length (degrees); only the spin period is configurable. */
#define SG_SPINNER_ARC_ANGLE 360

static JSValue js_getScreen(JSContext *ctx, JSValueConst this_val,
                             int argc, JSValueConst *argv) {
    (void)this_val; (void)argc; (void)argv;
    lv_obj_t *scr = lv_scr_act();
    /* Make the root reach the screen edges (theme adds padding by default) */
    lv_obj_set_style_pad_all(scr, 0, 0);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    return obj_to_js(ctx, scr);
}

static JSValue js_createNode(JSContext *ctx, JSValueConst this_val,
                              int argc, JSValueConst *argv) {
    (void)this_val;
    if (argc < 1) return JS_EXCEPTION;

    const char *type = JS_ToCString(ctx, argv[0]);
    if (!type) return JS_EXCEPTION;

    lv_obj_t *parent = lv_scr_act();
    lv_obj_t *obj    = NULL;

    if      (strcmp(type, "View")     == 0) { obj = lv_obj_create(parent); make_clean_container(obj); }
    else if (strcmp(type, "Text")     == 0) obj = lv_label_create(parent);
    else if (strcmp(type, "Button")   == 0) {
        obj = lv_button_create(parent);
        /* Every Button gets a child label by default */
        lv_obj_t *lbl = lv_label_create(obj);
        lv_label_set_text(lbl, "");
        lv_obj_center(lbl);
    }
    else if (strcmp(type, "Image")    == 0) obj = lv_image_create(parent);
    else if (strcmp(type, "Input")    == 0) {
        obj = lv_textarea_create(parent);
        /* Keep the IME candidate window anchored to this field */
        lv_obj_add_event_cb(obj, sg_ime_event_cb, LV_EVENT_FOCUSED, NULL);
        lv_obj_add_event_cb(obj, sg_ime_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    }
    else if (strcmp(type, "Switch")   == 0) obj = lv_switch_create(parent);
    else if (strcmp(type, "Progress") == 0) obj = lv_bar_create(parent);
    else if (strcmp(type, "Slider")   == 0) obj = lv_slider_create(parent);
    else if (strcmp(type, "Arc")      == 0) obj = lv_arc_create(parent);
    else if (strcmp(type, "Spinner")  == 0) {
        obj = lv_spinner_create(parent);
        lv_spinner_set_anim_params(obj, 10000, SG_SPINNER_ARC_ANGLE);
    }
    else if (strcmp(type, "Checkbox") == 0) {
        obj = lv_checkbox_create(parent);
        lv_checkbox_set_text(obj, "");
    }
    else if (strcmp(type, "Dropdown") == 0) obj = lv_dropdown_create(parent);
    else if (strcmp(type, "Roller")   == 0)
        obj = lv_roller_create(parent);
    else                                    { obj = lv_obj_create(parent); make_clean_container(obj); }

    /* Make interactive widgets reachable by the keyboard/encoder group */
    if (g_group && obj &&
        (strcmp(type, "Input")    == 0 ||
         strcmp(type, "Button")   == 0 ||
         strcmp(type, "Switch")   == 0 ||
         strcmp(type, "Slider")   == 0 ||
         strcmp(type, "Arc")      == 0 ||
         strcmp(type, "Checkbox") == 0 ||
         strcmp(type, "Dropdown") == 0 ||
         strcmp(type, "Roller")   == 0)) {
        lv_group_add_obj(g_group, obj);
    }

    JS_FreeCString(ctx, type);
    return obj_to_js(ctx, obj);
}

static JSValue js_appendChild(JSContext *ctx, JSValueConst this_val,
                               int argc, JSValueConst *argv) {
    (void)this_val; (void)ctx;
    if (argc < 2) return JS_EXCEPTION;
    lv_obj_t *parent = js_to_obj(ctx, argv[0]);
    lv_obj_t *child  = js_to_obj(ctx, argv[1]);
    lv_obj_set_parent(child, parent);
    return JS_UNDEFINED;
}

static JSValue js_removeChild(JSContext *ctx, JSValueConst this_val,
                               int argc, JSValueConst *argv) {
    (void)this_val; (void)ctx;
    if (argc < 2) return JS_EXCEPTION;
    lv_obj_t *child = js_to_obj(ctx, argv[1]);
    lv_obj_del(child);
    return JS_UNDEFINED;
}

static JSValue js_dispose(JSContext *ctx, JSValueConst this_val,
                           int argc, JSValueConst *argv) {
    (void)this_val; (void)ctx;
    if (argc < 1) return JS_EXCEPTION;
    lv_obj_del(js_to_obj(ctx, argv[0]));
    return JS_UNDEFINED;
}

/* setProperty(node, key, value) */
static JSValue js_setProperty(JSContext *ctx, JSValueConst this_val,
                               int argc, JSValueConst *argv) {
    (void)this_val;
    if (argc < 3) return JS_EXCEPTION;

    lv_obj_t   *obj = js_to_obj(ctx, argv[0]);
    const char *key = JS_ToCString(ctx, argv[1]);
    if (!key) return JS_EXCEPTION;

    /* ── layout / size ── */
    if (strcmp(key, "width") == 0) {
        lv_obj_set_width(obj, parse_size(ctx, argv[2]));
    } else if (strcmp(key, "height") == 0) {
        lv_obj_set_height(obj, parse_size(ctx, argv[2]));
    } else if (strcmp(key, "x") == 0) {
        int32_t v; JS_ToInt32(ctx, &v, argv[2]);
        lv_obj_set_x(obj, v);
    } else if (strcmp(key, "y") == 0) {
        int32_t v; JS_ToInt32(ctx, &v, argv[2]);
        lv_obj_set_y(obj, v);
    }
    /* ── flex layout ── */
    else if (strcmp(key, "flexFlow") == 0) {
        const char *v = JS_ToCString(ctx, argv[2]);
        lv_obj_set_layout(obj, LV_LAYOUT_FLEX);
        if (v && strcmp(v, "row") == 0)
            lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_ROW);
        else
            lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_COLUMN);
        JS_FreeCString(ctx, v);
    } else if (strcmp(key, "flexGrow") == 0) {
        int32_t v; JS_ToInt32(ctx, &v, argv[2]);
        lv_obj_set_flex_grow(obj, (uint8_t)v);
    }
    /* ── flex alignment ── */
    else if (strcmp(key, "alignItems") == 0 ||
             strcmp(key, "justifyContent") == 0) {
        const char *v = JS_ToCString(ctx, argv[2]);
        lv_flex_align_t a = LV_FLEX_ALIGN_START;
        if (v) {
            if      (strcmp(v, "center")  == 0) a = LV_FLEX_ALIGN_CENTER;
            else if (strcmp(v, "end")     == 0) a = LV_FLEX_ALIGN_END;
            else if (strcmp(v, "between") == 0) a = LV_FLEX_ALIGN_SPACE_BETWEEN;
            else if (strcmp(v, "around")  == 0) a = LV_FLEX_ALIGN_SPACE_AROUND;
            else if (strcmp(v, "evenly")  == 0) a = LV_FLEX_ALIGN_SPACE_EVENLY;
            else                                a = LV_FLEX_ALIGN_START;
        }
        /* alignItems → cross axis, justifyContent → main axis (CSS naming) */
        if (strcmp(key, "alignItems") == 0)
            lv_obj_set_style_flex_cross_place(obj, a, 0);
        else
            lv_obj_set_style_flex_main_place(obj, a, 0);
        JS_FreeCString(ctx, v);
    }
    /* ── flex gap (CSS `gap`): spacing between children, both axes ── */
    else if (strcmp(key, "gap") == 0) {
        int32_t v; JS_ToInt32(ctx, &v, argv[2]);
        lv_obj_set_style_pad_row(obj, v, 0);
        lv_obj_set_style_pad_column(obj, v, 0);
    }
    /* ── padding / margin ── */
    else if (strcmp(key, "padding") == 0) {
        int32_t v; JS_ToInt32(ctx, &v, argv[2]);
        lv_obj_set_style_pad_all(obj, v, 0);
    } else if (strcmp(key, "margin") == 0) {
        /* LVGL uses padding on parent; approximate via translation */
        (void)0;
    }
    /* ── background ── */
    else if (strcmp(key, "backgroundColor") == 0) {
        const char *v = JS_ToCString(ctx, argv[2]);
        sg_color_t c = parse_color_ex(v);
        lv_obj_set_style_bg_color(obj, c.color, 0);
        lv_obj_set_style_bg_opa(obj, c.opa, 0);
        JS_FreeCString(ctx, v);
    }
    /* ── border / radius ── */
    else if (strcmp(key, "borderRadius") == 0) {
        int32_t v; JS_ToInt32(ctx, &v, argv[2]);
        lv_obj_set_style_radius(obj, v, 0);
    } else if (strcmp(key, "borderWidth") == 0) {
        int32_t v; JS_ToInt32(ctx, &v, argv[2]);
        lv_obj_set_style_border_width(obj, v, 0);
    } else if (strcmp(key, "borderColor") == 0) {
        const char *v = JS_ToCString(ctx, argv[2]);
        sg_color_t c = parse_color_ex(v);
        lv_obj_set_style_border_color(obj, c.color, 0);
        lv_obj_set_style_border_opa(obj, c.opa, 0);
        JS_FreeCString(ctx, v);
    }
    /* ── text / label ── */
    else if (strcmp(key, "text") == 0) {
        const char *v = JS_ToCString(ctx, argv[2]);
        /* Support both lv_label and lv_button (update inner label) */
        const lv_obj_class_t *cls = lv_obj_get_class(obj);
        if (cls == &lv_label_class) {
            lv_label_set_text(obj, v ? v : "");
        } else if (cls == &lv_button_class) {
            lv_obj_t *lbl = lv_obj_get_child(obj, 0);
            if (lbl) lv_label_set_text(lbl, v ? v : "");
        } else if (cls == &lv_checkbox_class) {
            lv_checkbox_set_text(obj, v ? v : "");
        }
        JS_FreeCString(ctx, v);
    } else if (strcmp(key, "textColor") == 0) {
        const char *v = JS_ToCString(ctx, argv[2]);
        sg_color_t c = parse_color_ex(v);
        lv_obj_set_style_text_color(obj, c.color, 0);
        lv_obj_set_style_text_opa(obj, c.opa, 0);
        JS_FreeCString(ctx, v);
    } else if (strcmp(key, "fontSize") == 0) {
        int32_t v; JS_ToInt32(ctx, &v, argv[2]);
        /* Map to the nearest font compiled into lv_conf.h */
        const lv_font_t *f = &lv_font_montserrat_14;
        if      (v >= 24) f = &lv_font_montserrat_24;
        else if (v >= 20) f = &lv_font_montserrat_20;
        else if (v >= 16) f = &lv_font_montserrat_16;
        lv_obj_set_style_text_font(obj, f, 0);
    } else if (strcmp(key, "font") == 0) {
        /* int handle returned by loadFont() */
        int64_t h; JS_ToInt64(ctx, &h, argv[2]);
        if (h) lv_obj_set_style_text_font(obj, (const lv_font_t *)(uintptr_t)h, 0);
    } else if (strcmp(key, "scrollable") == 0) {
        if (JS_ToBool(ctx, argv[2]))
            lv_obj_add_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
        else
            lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    }
    /* ── Image ── */
    else if (strcmp(key, "src") == 0) {
        const char *v = JS_ToCString(ctx, argv[2]);
        const lv_obj_class_t *cls = lv_obj_get_class(obj);
        if (cls == &lv_image_class && v)
            lv_image_set_src(obj, v);
        JS_FreeCString(ctx, v);
    }
    /* ── Progress / bar / slider / arc / dropdown / roller ── */
    else if (strcmp(key, "value") == 0) {
        int32_t v; JS_ToInt32(ctx, &v, argv[2]);
        const lv_obj_class_t *cls = lv_obj_get_class(obj);
        if      (cls == &lv_bar_class)      lv_bar_set_value(obj, v, LV_ANIM_OFF);
        else if (cls == &lv_slider_class)   lv_slider_set_value(obj, v, LV_ANIM_OFF);
        else if (cls == &lv_arc_class)      lv_arc_set_value(obj, v);
        else if (cls == &lv_dropdown_class) lv_dropdown_set_selected(obj, (uint32_t)v);
        else if (cls == &lv_roller_class)   lv_roller_set_selected(obj, (uint32_t)v, LV_ANIM_OFF);
    } else if (strcmp(key, "min") == 0) {
        int32_t v; JS_ToInt32(ctx, &v, argv[2]);
        const lv_obj_class_t *cls = lv_obj_get_class(obj);
        if (cls == &lv_bar_class)
            lv_bar_set_range(obj, v, lv_bar_get_max_value(obj));
        else if (cls == &lv_slider_class)
            lv_slider_set_range(obj, v, lv_slider_get_max_value(obj));
        else if (cls == &lv_arc_class)
            lv_arc_set_range(obj, v, lv_arc_get_max_value(obj));
    } else if (strcmp(key, "max") == 0) {
        int32_t v; JS_ToInt32(ctx, &v, argv[2]);
        const lv_obj_class_t *cls = lv_obj_get_class(obj);
        if (cls == &lv_bar_class)
            lv_bar_set_range(obj, lv_bar_get_min_value(obj), v);
        else if (cls == &lv_slider_class)
            lv_slider_set_range(obj, lv_slider_get_min_value(obj), v);
        else if (cls == &lv_arc_class)
            lv_arc_set_range(obj, lv_arc_get_min_value(obj), v);
    }
    /* ── Dropdown / Roller options ("a\nb\nc") ── */
    else if (strcmp(key, "options") == 0) {
        const char *v = JS_ToCString(ctx, argv[2]);
        const lv_obj_class_t *cls = lv_obj_get_class(obj);
        if (v) {
            if (cls == &lv_dropdown_class)
                lv_dropdown_set_options(obj, v);
            else if (cls == &lv_roller_class)
                lv_roller_set_options(obj, v, LV_ROLLER_MODE_NORMAL);
        }
        JS_FreeCString(ctx, v);
    }
    /* ── Switch / Checkbox ── */
    else if (strcmp(key, "checked") == 0) {
        int v = JS_ToBool(ctx, argv[2]);
        if (v) lv_obj_add_state(obj, LV_STATE_CHECKED);
        else   lv_obj_remove_state(obj, LV_STATE_CHECKED);
    }
    /* ── Input / textarea ── */
    else if (strcmp(key, "placeholder") == 0) {
        const char *v = JS_ToCString(ctx, argv[2]);
        const lv_obj_class_t *cls = lv_obj_get_class(obj);
        if (cls == &lv_textarea_class)
            lv_textarea_set_placeholder_text(obj, v ? v : "");
        JS_FreeCString(ctx, v);
    }
    /* ── Spinner: spinTime (ms per revolution). Arc length is a fixed
     * default; lv_spinner_set_anim_params() takes both at once. */
    else if (strcmp(key, "spinTime") == 0) {
        const lv_obj_class_t *cls = lv_obj_get_class(obj);
        if (cls == &lv_spinner_class) {
            int32_t v; JS_ToInt32(ctx, &v, argv[2]);
            if (v <= 0) v = 1000;
            lv_spinner_set_anim_params(obj, (uint32_t)v, SG_SPINNER_ARC_ANGLE);
        }
    }

    JS_FreeCString(ctx, key);
    return JS_UNDEFINED;
}

/* getProperty(node, key) — read back common widget state.
 *   "value"   → number (slider/bar/arc) or selected index (dropdown/roller),
 *               or bool (switch/checkbox), or string (input)
 *   "checked" → bool   (switch/checkbox)
 *   "text"    → string (input/label)
 */
static JSValue js_getProperty(JSContext *ctx, JSValueConst this_val,
                               int argc, JSValueConst *argv) {
    (void)this_val;
    if (argc < 2) return JS_EXCEPTION;

    lv_obj_t   *obj = js_to_obj(ctx, argv[0]);
    const char *key = JS_ToCString(ctx, argv[1]);
    if (!key) return JS_EXCEPTION;

    JSValue out;
    if (strcmp(key, "checked") == 0) {
        out = JS_NewBool(ctx, lv_obj_has_state(obj, LV_STATE_CHECKED));
    } else if (strcmp(key, "text") == 0) {
        const lv_obj_class_t *cls = lv_obj_get_class(obj);
        if (cls == &lv_textarea_class)
            out = JS_NewString(ctx, lv_textarea_get_text(obj));
        else if (cls == &lv_label_class)
            out = JS_NewString(ctx, lv_label_get_text(obj));
        else
            out = JS_UNDEFINED;
    } else {
        /* "value" and anything else → the widget's natural value */
        out = widget_value_to_js(ctx, obj);
    }

    JS_FreeCString(ctx, key);
    return out;
}

/* addEvent(node, eventName, callback) */
static JSValue js_addEvent(JSContext *ctx, JSValueConst this_val,
                            int argc, JSValueConst *argv) {
    (void)this_val;
    if (argc < 3) return JS_EXCEPTION;

    lv_obj_t   *obj  = js_to_obj(ctx, argv[0]);
    const char *name = JS_ToCString(ctx, argv[1]);
    if (!name) return JS_EXCEPTION;

    lv_event_code_t code = LV_EVENT_CLICKED;
    if      (strcmp(name, "click")     == 0) code = LV_EVENT_CLICKED;
    else if (strcmp(name, "longpress") == 0) code = LV_EVENT_LONG_PRESSED;
    else if (strcmp(name, "change")    == 0) code = LV_EVENT_VALUE_CHANGED;
    else if (strcmp(name, "focus")     == 0) code = LV_EVENT_FOCUSED;
    else if (strcmp(name, "blur")      == 0) code = LV_EVENT_DEFOCUSED;

    JS_FreeCString(ctx, name);

    EventCb *ecb = lv_malloc(sizeof(EventCb));
    ecb->ctx = ctx;
    ecb->fn  = JS_DupValue(ctx, argv[2]);

    lv_obj_add_event_cb(obj, lv_event_dispatch, code, ecb);
    /* Free the JS handler + struct when the widget is destroyed */
    lv_obj_add_event_cb(obj, lv_event_free, LV_EVENT_DELETE, ecb);
    return JS_UNDEFINED;
}

/* loadFont(path, size) → font handle (int), or 0 on failure.
 * Reads the TTF/TTC file fully into memory (kept alive for the font lifetime)
 * and builds a Tiny-TTF font. Supports CJK and arbitrary sizes. */
static JSValue js_loadFont(JSContext *ctx, JSValueConst this_val,
                            int argc, JSValueConst *argv) {
    (void)this_val;
    if (argc < 2) return JS_EXCEPTION;

    const char *path = JS_ToCString(ctx, argv[0]);
    int32_t size; JS_ToInt32(ctx, &size, argv[1]);
    if (!path) return JS_EXCEPTION;

    JSValue result = JS_NewInt64(ctx, 0);
    FILE *f = fopen(path, "rb");
    if (f) {
        fseek(f, 0, SEEK_END);
        long len = ftell(f);
        fseek(f, 0, SEEK_SET);
        if (len > 0) {
            /* Intentionally not freed: Tiny-TTF references this buffer. */
            uint8_t *buf = lv_malloc((size_t)len);
            if (buf && fread(buf, 1, (size_t)len, f) == (size_t)len) {
                lv_font_t *font = lv_tiny_ttf_create_data(buf, (size_t)len, size);
                if (font) {
                    /* CJK TTF files have no glyphs for LVGL's built-in icon
                     * symbols (LV_SYMBOL_OK/DOWN/… live in the 0xF000+ private
                     * use area, supplied only by Montserrat). Fall back to the
                     * nearest enabled Montserrat so checkbox ticks, dropdown
                     * arrows, etc. render instead of showing tofu boxes. */
                    const lv_font_t *fb = &lv_font_montserrat_14;
                    if      (size >= 24) fb = &lv_font_montserrat_24;
                    else if (size >= 20) fb = &lv_font_montserrat_20;
                    else if (size >= 16) fb = &lv_font_montserrat_16;
                    font->fallback = fb;
                    result = obj_to_js(ctx, (lv_obj_t *)font);
                } else {
                    lv_free(buf);
                }
            } else if (buf) {
                lv_free(buf);
            }
        }
        fclose(f);
    }
    JS_FreeCString(ctx, path);
    return result;
}

/* setDefaultFont(handle) — make a loaded font the global default while keeping
 * stonegui's custom theme. All widgets that don't override `font` inherit it. */
static JSValue js_setDefaultFont(JSContext *ctx, JSValueConst this_val,
                                  int argc, JSValueConst *argv) {
    (void)this_val;
    if (argc < 1) return JS_EXCEPTION;
    int64_t h; JS_ToInt64(ctx, &h, argv[0]);
    if (!h) return JS_UNDEFINED;

    sg_theme_set_font(lv_display_get_default(), (const lv_font_t *)(uintptr_t)h);
    return JS_UNDEFINED;
}

/* ── module export list ─────────────────────────────────────────────────── */

static const JSCFunctionListEntry lv_funcs[] = {
    JS_CFUNC_DEF("getScreen",    0, js_getScreen),
    JS_CFUNC_DEF("createNode",   1, js_createNode),
    JS_CFUNC_DEF("appendChild",  2, js_appendChild),
    JS_CFUNC_DEF("removeChild",  2, js_removeChild),
    JS_CFUNC_DEF("setProperty",  3, js_setProperty),
    JS_CFUNC_DEF("getProperty",  2, js_getProperty),
    JS_CFUNC_DEF("addEvent",     3, js_addEvent),
    JS_CFUNC_DEF("dispose",      1, js_dispose),
    JS_CFUNC_DEF("loadFont",     2, js_loadFont),
    JS_CFUNC_DEF("setDefaultFont", 1, js_setDefaultFont),
};

static int lv_module_init(JSContext *ctx, JSModuleDef *m) {
    return JS_SetModuleExportList(ctx, m, lv_funcs,
                                  sizeof(lv_funcs) / sizeof(lv_funcs[0]));
}

JSModuleDef *js_init_module_lvgl(JSContext *ctx, const char *module_name) {
    JSModuleDef *m = JS_NewCModule(ctx, module_name, lv_module_init);
    if (!m) return NULL;
    JS_AddModuleExportList(ctx, m, lv_funcs,
                           sizeof(lv_funcs) / sizeof(lv_funcs[0]));
    return m;
}

void lv_bindings_register(JSContext *ctx) {
    js_init_module_lvgl(ctx, "lvgl");
}

void lv_bindings_flush_callbacks(JSContext *ctx) {
    /* QuickJS pending jobs (Promise continuations etc.) */
    JSContext *ctx1;
    int ret;
    do {
        ret = JS_ExecutePendingJob(JS_GetRuntime(ctx), &ctx1);
    } while (ret > 0);
}
