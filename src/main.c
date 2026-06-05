/**
 * main.c — stonegui host
 *
 * Initialises:
 *   1. LVGL
 *   2. LVGL SDL display + mouse/keyboard inputs
 *   3. QuickJS runtime
 *   4. Native "lvgl" module bindings
 *   5. Loads and executes the JS bundle
 *   6. Runs the event loop
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "lvgl.h"
#include "src/drivers/sdl/lv_sdl_window.h"
#include "src/drivers/sdl/lv_sdl_mouse.h"
#include "src/drivers/sdl/lv_sdl_keyboard.h"

#include "quickjs.h"
#include "quickjs-libc.h"

#include "lv_bindings.h"

#define DISPLAY_WIDTH  800
#define DISPLAY_HEIGHT 480
#define BUNDLE_PATH    "examples/hello/app.js"

/* ── LVGL tick source ───────────────────────────────────────────────────── */

static uint32_t millis(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

/* ── load a JS module file ──────────────────────────────────────────────── */

static int load_and_run(JSContext *ctx, const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "stonegui: cannot open '%s'\n", path);
        return -1;
    }
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc(len + 1);
    if (!buf) { fclose(f); return -1; }
    fread(buf, 1, len, f);
    buf[len] = '\0';
    fclose(f);

    JSValue val = JS_Eval(ctx, buf, len, path,
                          JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
    free(buf);
    if (JS_IsException(val)) {
        js_std_dump_error(ctx);
        return -1;
    }

    /* link and evaluate the module */
    JSValue promise = JS_EvalFunction(ctx, val);
    if (JS_IsException(promise)) {
        js_std_dump_error(ctx);
        return -1;
    }
    js_std_loop(ctx);   /* drain initial micro-task queue */
    JS_FreeValue(ctx, promise);
    return 0;
}

/* ── main ───────────────────────────────────────────────────────────────── */

int main(int argc, char *argv[]) {
    const char *bundle = (argc > 1) ? argv[1] : BUNDLE_PATH;

    /* 1. Initialise LVGL */
    lv_init();

    /* 2. Create SDL display */
    lv_display_t *disp = lv_sdl_window_create(DISPLAY_WIDTH, DISPLAY_HEIGHT);
    if (!disp) {
        fprintf(stderr, "stonegui: failed to create SDL window\n");
        return 1;
    }
    lv_display_set_default(disp);

    /* 3. Input devices */
    lv_indev_t *mouse = lv_sdl_mouse_create();
    (void)mouse;
    lv_indev_t *kb = lv_sdl_keyboard_create();
    (void)kb;

    /* 4. QuickJS runtime */
    JSRuntime *rt = JS_NewRuntime();
    js_std_set_worker_new_context_func(NULL);
    js_std_init_handlers(rt);

    JSContext *ctx = JS_NewContext(rt);
    js_std_add_helpers(ctx, 0, NULL);
    js_init_module_std(ctx, "std");
    js_init_module_os(ctx, "os");

    /* ES module loader so relative imports ("../../js/framework.js") resolve */
    JS_SetModuleLoaderFunc2(rt, NULL, js_module_loader,
                            js_module_check_attributes, NULL);

    /* 5. Native LVGL module */
    lv_bindings_register(ctx);

    /* 6. Load and execute the application bundle */
    if (load_and_run(ctx, bundle) != 0) {
        fprintf(stderr, "stonegui: failed to load bundle: %s\n", bundle);
        JS_FreeContext(ctx);
        JS_FreeRuntime(rt);
        return 1;
    }

    printf("stonegui: running %s\n", bundle);

    /* 7. Event loop */
    uint32_t last_tick = millis();
    while (1) {
        /* LVGL tick */
        uint32_t now  = millis();
        uint32_t diff = now - last_tick;
        last_tick     = now;
        lv_tick_inc(diff);

        /* Handle LVGL tasks (SDL events are polled inside) */
        uint32_t sleep_ms = lv_timer_handler();

        /* Flush any pending JS microtasks / Promise callbacks */
        lv_bindings_flush_callbacks(ctx);

        /* Yield to OS */
        if (sleep_ms > 0 && sleep_ms < 32)
            usleep(sleep_ms * 1000);
        else
            usleep(5000);
    }

    JS_FreeContext(ctx);
    JS_FreeRuntime(rt);
    return 0;
}
