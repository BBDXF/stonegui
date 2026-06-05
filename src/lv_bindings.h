#ifndef LV_BINDINGS_H
#define LV_BINDINGS_H

#include "quickjs.h"
#include "lvgl.h"

/* Register the native "lvgl" module into the QuickJS runtime */
void lv_bindings_register(JSContext *ctx);

/* Set the input group that focusable widgets are added to (keyboard focus) */
void lv_bindings_set_group(lv_group_t *group);

/* Must be called from LVGL event loop to dispatch pending JS callbacks */
void lv_bindings_flush_callbacks(JSContext *ctx);

#endif /* LV_BINDINGS_H */
