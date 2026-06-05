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

In JSX these are the lowercase host tags (`view`, `text`, …); the canonical
names above also work. You can define your own **function components** that take
props and `children` and return elements, and group siblings with a `Fragment`
(`<>…</>`).

`View` is a transparent, borderless, zero-padding layout box (React-Native
semantics) — the default LVGL theme decorations are stripped, so a `View` only
shows what you style. Set `backgroundColor`, `borderRadius`, `padding`, etc.
explicitly when you want them.

### Supported props / style keys

* Layout: `width`, `height` (px number, or `"100%"` / `"fill"`), `x`, `y`,
  `flexFlow` (`"row"`/`"column"`), `flexGrow`, `padding`, `scrollable`
* Appearance: `backgroundColor`, `borderRadius`, `borderWidth`, `borderColor`,
  `textColor`, `fontSize` (14/16/20/24), `font` (handle from `loadFont`)
* Content: `text`, `placeholder`, `src` (Image), `value`/`min`/`max` (Progress),
  `checked` (Switch)
* Events: `onClick`, `onLongPress`, `onChange`, `onFocus`, `onBlur`

Any prop or style value may be a function (`() => signal()`) to make it reactive.

### Fonts & internationalization (CJK / Chinese)

The built-in Montserrat font is Latin-only. To render Chinese (or any other
script) load a TTF/TTC at runtime. The recommended pattern is a **global default
font**, overriding per-element only when you need a different size/face:

```js
import { loadFont, setDefaultFont } from "../../js/framework.js";

const body  = loadFont("/usr/share/fonts/truetype/wqy/wqy-microhei.ttc", 20);
const title = loadFont("/usr/share/fonts/truetype/wqy/wqy-microhei.ttc", 30);

setDefaultFont(body);                                   // global default
h("Text", { text: "你好，世界" });                       // inherits `body`
h("Text", { style: { font: title }, text: "标题" });    // explicit override
```

* `loadFont(path, size)` reads the file into memory and builds a Tiny-TTF font;
  returns `0` if the file can't be opened. Text encoding is UTF-8.
* `setDefaultFont(handle)` re-applies the theme with that font as the default, so
  every widget inherits it unless it sets its own `font`.
* Tiny-TTF fonts are a fixed pixel size per handle — load one handle per size you
  need. (`fontSize` only selects the built-in Latin Montserrat sizes.)

### Keyboard input

A global input group is wired to the keyboard device. `Input`, `Button` and
`Switch` widgets are added to it automatically, so you can **click an `Input`
to focus it and then type**, or `Tab` between focusable widgets.

ASCII and multi-byte **UTF-8 (CJK) input** both work: the host installs a
UTF-8-aware keyboard read callback that delivers one whole character per
keypress (LVGL's default SDL driver otherwise splits multi-byte characters and
corrupts them).

To actually compose Chinese you still need a system **IME** (e.g. fcitx5 or
ibus) running and active — SDL forwards the IME's composed text to the app.
With no IME installed, the OS only sends raw Latin keystrokes.

Each `Input` reports its on-screen rectangle to SDL (`SDL_SetTextInputRect`) when
focused or edited, so the IME's candidate (composition) window follows the input
box instead of appearing at the screen origin.

### Responsive layout

The SDL window is resizable. Use percent sizes (`"100%"`) and `flexGrow` so the
UI reflows when the window is resized.

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

## Examples

### `examples/hello` — hyperscript

Uses the `h(type, props, …children)` factory directly. No build step; just run:

```sh
./build/stonegui examples/hello/app.js
```

### `examples/jsx` — JSX (React/Vue style)

Real JSX, transpiled to plain JS with esbuild (QuickJS can't parse JSX itself).

```sh
cd examples/jsx
npm install        # once — installs esbuild (dev dependency)
npm run build      # app.jsx → app.js  (or: npm run watch)
cd ../..
./build/stonegui examples/jsx/app.js
```

It shows function components with props/`children`, fragments (`<>…</>`), list
rendering with `.map()`, signals/events, and Chinese text. Conventions follow
React DOM: **lowercase tags are host widgets** (`view`, `text`, `button`,
`switch`, `progress`, `input`, `image`) and **Capitalized tags are components**.

`node_modules/` is gitignored — `app.js` is committed so the demo runs without
installing anything; you only need `npm install` to re-build after editing
`app.jsx`.

## Status

MVP on LVGL 9.x + QuickJS + SDL2 (Linux). Implemented: the components above,
style props, click/value events, signals with fine-grained updates, ES module
loading, keyboard input (focus group), runtime TTF/CJK fonts, and a resizable
window with percent-based responsive sizing. Planned next: hot reload,
framework adapters (Solid/Preact), and a Wayland backend.
