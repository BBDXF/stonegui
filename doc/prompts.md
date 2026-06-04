# Project: LVUI (React Native for LVGL)

## Goal

Build a modern embedded GUI framework on top of:

* LVGL 9.x
* Wayland (production)
* SDL (development)
* QuickJS
* TypeScript
* JSX

The framework should provide a React-like / Solid-like developer experience while rendering directly to LVGL widgets.

This is NOT a browser framework.

This is NOT an XML renderer.

This is NOT a wrapper around LVGL APIs.

The goal is to create a declarative UI framework similar to:

* React Native
* Flutter
* Jetpack Compose

but using LVGL as the rendering backend.

---

# Core Architecture

```text
Application
    ↓
SolidJS / Preact
    ↓
Framework Adapter
    ↓
LVGL Renderer Core
    ↓
LVGL Native Objects
    ↓
Wayland / SDL
```

Important:

The renderer must operate directly on LVGL objects.

DO NOT generate XML.

DO NOT generate HTML.

DO NOT serialize UI trees.

Renderer updates should map directly to LVGL API calls.

Example:

```tsx
<Label text={cpu()} />
```

should become:

```c
lv_label_set_text(...)
```

instead of:

```text
JSX
 → XML
 → XML Parser
 → LVGL
```

---

# Design Principles

## Principle 1

LVGL is the rendering engine.

The framework owns:

* component model
* state model
* reconciliation
* lifecycle

LVGL owns:

* widgets
* layouts
* styles
* animations
* events
* rendering

---

## Principle 2

Renderer Core must be framework-agnostic.

The renderer should not depend on:

* React
* Preact
* Solid

Adapters should be separate packages.

Example:

```text
packages/

lvgl-renderer-core

lvgl-renderer-solid

lvgl-renderer-preact
```

---

## Principle 3

The renderer API should be minimal.

Example:

```ts
interface HostRenderer {
    createNode(type)
    appendChild(parent, child)
    removeChild(node)

    setProperty(node, key, value)

    addEvent(node, event, handler)

    dispose(node)
}
```

Framework adapters communicate only through this API.

---

# Component Model

Provide React Native style components.

Example:

```tsx
<View />
<Text />
<Button />
<Image />
<Input />
<Switch />
<Progress />
<List />
```

Mapping:

```text
View        -> lv_obj
Text        -> lv_label
Button      -> lv_button
Image       -> lv_image
Input       -> lv_textarea
Switch      -> lv_switch
Progress    -> lv_bar
List        -> lv_list
```

Applications should never call LVGL directly.

---

# Styling System

Use React Native style objects.

Example:

```tsx
<Button
  style={{
      width: 120,
      height: 40,
      backgroundColor: "#ff0000",
      borderRadius: 8
  }}
/>
```

Renderer translates styles into LVGL style APIs.

Example:

```c
lv_obj_set_size(...)
lv_obj_set_style_bg_color(...)
lv_obj_set_style_radius(...)
```

Do not implement CSS parsing.

Do not implement CSS selectors.

Style objects only.

---

# Event System

Example:

```tsx
<Button
    onClick={start}
    onLongPress={stop}
/>
```

Maps to:

```c
LV_EVENT_CLICKED
LV_EVENT_LONG_PRESSED
```

Events must flow:

```text
LVGL
  ↓
Renderer
  ↓
JS Runtime
```

---

# State Management

Design renderer to support two frontends:

## Option A

Preact + Signals

## Option B

SolidJS + Signals

Renderer Core must support both.

Do not assume React Fiber.

Do not assume Virtual DOM.

The renderer should work with fine-grained updates.

Example:

```tsx
const [rpm, setRpm] = createSignal(0)

<Gauge value={rpm()} />
```

When rpm changes:

```text
update property only
```

not:

```text
destroy widget
recreate widget
```

---

# Rendering Rules

Widget creation:

```tsx
<Button />
```

becomes:

```c
lv_button_create(...)
```

Property update:

```tsx
<Button text="Start" />
```

becomes:

```c
lv_label_set_text(...)
```

Tree updates:

```tsx
<View>
   <Button />
</View>
```

becomes:

```c
lv_obj_set_parent(...)
```

No XML.

No serialization.

No runtime parser.

---

# Runtime

QuickJS is the embedded runtime.

Requirements:

* load JS bundles
* support ES modules
* support hot reload
* support filesystem API
* support network API

Future:

* OTA bundle update
* remote UI deployment

---

# Development Backend

Use SDL.

```text
QuickJS
 ↓
Renderer
 ↓
LVGL
 ↓
SDL
```

---

# Production Backend

Use Wayland.

```text
QuickJS
 ↓
Renderer
 ↓
LVGL
 ↓
Wayland
```

---

# Repository Structure

```text
packages/

lvgl-renderer-core/
lvgl-renderer-solid/
lvgl-renderer-preact/

lvui-components/
lvui-runtime/
lvui-devtools/

examples/
```

---

# MVP

Implement:

* View
* Text
* Button
* Image
* Input
* Progress

Support:

* style props
* click events
* signals
* hot reload

Run:

```text
QuickJS
+
LVGL
+
SDL
```

on Linux.

Wayland support can be added after MVP succeeds.
