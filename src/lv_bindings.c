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

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "lvgl.h"
#include "quickjs-libc.h"

/* ── helpers ────────────────────────────────────────────────────────────── */

static lv_obj_t *js_to_obj(JSContext *ctx, JSValueConst v) {
    int64_t ptr;
    JS_ToInt64(ctx, &ptr, v);
    return (lv_obj_t *)(uintptr_t)ptr;
}

static JSValue obj_to_js(JSContext *ctx, lv_obj_t *obj) {
    return JS_NewInt64(ctx, (int64_t)(uintptr_t)obj);
}

/* colour string "#rrggbb" → lv_color_t */
static lv_color_t parse_color(const char *s) {
    if (!s || s[0] != '#') return lv_color_black();
    unsigned int r = 0, g = 0, b = 0;
    sscanf(s + 1, "%02x%02x%02x", &r, &g, &b);
    return lv_color_make(r, g, b);
}

/* ── event callback bridge ──────────────────────────────────────────────── */

typedef struct {
    JSContext *ctx;
    JSValue    fn;
} EventCb;

static void lv_event_dispatch(lv_event_t *e) {
    EventCb *ecb = (EventCb *)lv_event_get_user_data(e);
    if (!ecb) return;
    JSValue ret = JS_Call(ecb->ctx, ecb->fn, JS_UNDEFINED, 0, NULL);
    if (JS_IsException(ret))
        js_std_dump_error(ecb->ctx);
    JS_FreeValue(ecb->ctx, ret);
}

/* ── module functions ───────────────────────────────────────────────────── */

static JSValue js_getScreen(JSContext *ctx, JSValueConst this_val,
                             int argc, JSValueConst *argv) {
    (void)this_val; (void)argc; (void)argv;
    return obj_to_js(ctx, lv_scr_act());
}

static JSValue js_createNode(JSContext *ctx, JSValueConst this_val,
                              int argc, JSValueConst *argv) {
    (void)this_val;
    if (argc < 1) return JS_EXCEPTION;

    const char *type = JS_ToCString(ctx, argv[0]);
    if (!type) return JS_EXCEPTION;

    lv_obj_t *parent = lv_scr_act();
    lv_obj_t *obj    = NULL;

    if      (strcmp(type, "View")     == 0) obj = lv_obj_create(parent);
    else if (strcmp(type, "Text")     == 0) obj = lv_label_create(parent);
    else if (strcmp(type, "Button")   == 0) {
        obj = lv_button_create(parent);
        /* Every Button gets a child label by default */
        lv_obj_t *lbl = lv_label_create(obj);
        lv_label_set_text(lbl, "");
        lv_obj_center(lbl);
    }
    else if (strcmp(type, "Image")    == 0) obj = lv_image_create(parent);
    else if (strcmp(type, "Input")    == 0) obj = lv_textarea_create(parent);
    else if (strcmp(type, "Switch")   == 0) obj = lv_switch_create(parent);
    else if (strcmp(type, "Progress") == 0) obj = lv_bar_create(parent);
    else                                    obj = lv_obj_create(parent);

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
        int32_t v; JS_ToInt32(ctx, &v, argv[2]);
        lv_obj_set_width(obj, v);
    } else if (strcmp(key, "height") == 0) {
        int32_t v; JS_ToInt32(ctx, &v, argv[2]);
        lv_obj_set_height(obj, v);
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
        lv_obj_set_style_bg_color(obj, parse_color(v), 0);
        lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
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
        lv_obj_set_style_border_color(obj, parse_color(v), 0);
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
        }
        JS_FreeCString(ctx, v);
    } else if (strcmp(key, "textColor") == 0) {
        const char *v = JS_ToCString(ctx, argv[2]);
        lv_obj_set_style_text_color(obj, parse_color(v), 0);
        JS_FreeCString(ctx, v);
    } else if (strcmp(key, "fontSize") == 0) {
        /* font size switching not trivially dynamic in LVGL; skip for MVP */
        (void)0;
    }
    /* ── Progress / bar ── */
    else if (strcmp(key, "value") == 0) {
        int32_t v; JS_ToInt32(ctx, &v, argv[2]);
        const lv_obj_class_t *cls = lv_obj_get_class(obj);
        if (cls == &lv_bar_class)
            lv_bar_set_value(obj, v, LV_ANIM_OFF);
    } else if (strcmp(key, "min") == 0) {
        int32_t v; JS_ToInt32(ctx, &v, argv[2]);
        const lv_obj_class_t *cls = lv_obj_get_class(obj);
        if (cls == &lv_bar_class) {
            int32_t max = lv_bar_get_max_value(obj);
            lv_bar_set_range(obj, v, max);
        }
    } else if (strcmp(key, "max") == 0) {
        int32_t v; JS_ToInt32(ctx, &v, argv[2]);
        const lv_obj_class_t *cls = lv_obj_get_class(obj);
        if (cls == &lv_bar_class) {
            int32_t min = lv_bar_get_min_value(obj);
            lv_bar_set_range(obj, min, v);
        }
    }
    /* ── Switch ── */
    else if (strcmp(key, "checked") == 0) {
        int v = JS_ToBool(ctx, argv[2]);
        const lv_obj_class_t *cls = lv_obj_get_class(obj);
        if (cls == &lv_switch_class) {
            if (v) lv_obj_add_state(obj, LV_STATE_CHECKED);
            else   lv_obj_clear_state(obj, LV_STATE_CHECKED);
        }
    }
    /* ── Input / textarea ── */
    else if (strcmp(key, "placeholder") == 0) {
        const char *v = JS_ToCString(ctx, argv[2]);
        const lv_obj_class_t *cls = lv_obj_get_class(obj);
        if (cls == &lv_textarea_class)
            lv_textarea_set_placeholder_text(obj, v ? v : "");
        JS_FreeCString(ctx, v);
    }

    JS_FreeCString(ctx, key);
    return JS_UNDEFINED;
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
    return JS_UNDEFINED;
}

/* ── module export list ─────────────────────────────────────────────────── */

static const JSCFunctionListEntry lv_funcs[] = {
    JS_CFUNC_DEF("getScreen",    0, js_getScreen),
    JS_CFUNC_DEF("createNode",   1, js_createNode),
    JS_CFUNC_DEF("appendChild",  2, js_appendChild),
    JS_CFUNC_DEF("removeChild",  2, js_removeChild),
    JS_CFUNC_DEF("setProperty",  3, js_setProperty),
    JS_CFUNC_DEF("addEvent",     3, js_addEvent),
    JS_CFUNC_DEF("dispose",      1, js_dispose),
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
