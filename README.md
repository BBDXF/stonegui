# stonegui

A modern, declarative embedded GUI framework: **QuickJS + LVGL**, rendering
directly to native LVGL widgets — no XML, no HTML, no serialized UI tree.

You write React-Native-style components in JavaScript; the renderer maps them
straight to LVGL API calls.

```js
h("Text", { text: () => `Count: ${count()}` })
//                         ↓ on signal change
// lv_label_set_text(...)      // a single property update, not a remount
```

See [`doc/prompts.md`](doc/prompts.md) for the full design vision.

## Architecture

```text
Application (examples/*)
    ↓
framework.js          fine-grained reactivity (signals + effects)
    ↓  HostRenderer API (createNode / appendChild / setProperty / addEvent / dispose)
lv_bindings.c         native "lvgl" QuickJS module
    ↓
LVGL                  widgets, layout, styles, events, rendering
    ↓
SDL (dev)  /  Wayland (production)
```

* **`js/framework.js`** — framework-agnostic reactive core. The component tree
  is built **once**. Reactive values are passed as accessor functions (thunks);
  each reactive prop owns its own effect, so a signal change updates a single
  LVGL property. Widgets are never destroyed and recreated.
* **`src/lv_bindings.c`** — the minimal `HostRenderer` surface exposed to JS as
  the native `lvgl` module.
* **`src/main.c`** — host: boots LVGL + SDL, the QuickJS runtime (with the ES
  module loader), loads the bundle, and runs the event loop.

## Components

| Component  | LVGL widget   |
| ---------- | ------------- |
| `View`     | `lv_obj`      |
| `Text`     | `lv_label`    |
| `Button`   | `lv_button`   |
| `Image`    | `lv_image`    |
| `Input`    | `lv_textarea` |
| `Switch`   | `lv_switch`   |
| `Progress` | `lv_bar`      |

### Supported props / style keys

* Layout: `width`, `height`, `x`, `y`, `flexFlow` (`"row"`/`"column"`),
  `flexGrow`, `padding`
* Appearance: `backgroundColor`, `borderRadius`, `borderWidth`, `borderColor`,
  `textColor`, `fontSize` (14/16/20/24)
* Content: `text`, `placeholder`, `src` (Image), `value`/`min`/`max` (Progress),
  `checked` (Switch)
* Events: `onClick`, `onLongPress`, `onChange`, `onFocus`, `onBlur`

Any prop or style value may be a function (`() => signal()`) to make it reactive.

## Build & run

Requires CMake, a C/C++ toolchain, and SDL2 (`pkg-config sdl2`).

```sh
cmake -S . -B build
cmake --build build
./build/stonegui                       # runs examples/hello/app.js
./build/stonegui examples/hello/app.js # or pass a bundle explicitly
```

The build copies `js/` and `examples/` next to the binary so relative imports
resolve at runtime.

## Status

MVP on LVGL 9.x + QuickJS + SDL2 (Linux). Implemented: the components above,
style props, click/value events, signals with fine-grained updates, and ES
module loading. Planned next: hot reload, framework adapters
(Solid/Preact), and a Wayland backend.
