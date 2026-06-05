/**
 * framework.js — Minimal fine-grained reactive UI framework for stonegui
 *
 * Provides:
 *   createSignal(init)          → [get, set]
 *   createEffect(fn)            → auto-tracks signals read inside fn
 *   h(type, props, ...children) → builds a VNode tree
 *   render(rootFn, container)   → mounts a component tree (once)
 *
 * Design (see doc/prompts.md — State Management):
 *   The tree is built ONCE. Reactive values are passed as accessor functions
 *   (thunks). Each reactive prop owns its own effect, so a signal change maps
 *   to a single `lv.setProperty(...)` call — the widget is never destroyed and
 *   recreated.
 *
 *     <Text text={() => `Count: ${count()}`} />
 *        → lv_label_set_text(...)   // property update only
 */

import * as lv from "lvgl";

/* ── Reactivity ─────────────────────────────────────────────────────────── */

let currentEffect = null;

export function createSignal(init) {
    let value = init;
    const subscribers = new Set();

    const read = () => {
        if (currentEffect) subscribers.add(currentEffect);
        return value;
    };

    const write = (next) => {
        const v = typeof next === "function" ? next(value) : next;
        if (v === value) return;
        value = v;
        for (const sub of [...subscribers]) sub();
    };

    return [read, write];
}

export function createEffect(fn) {
    const run = () => {
        const prev = currentEffect;
        currentEffect = run;
        try { fn(); } finally { currentEffect = prev; }
    };
    run();
}

/* ── VNode ──────────────────────────────────────────────────────────────── */

/* A VNode describes one piece of UI before it is mounted to a native node. */
class VNode {
    constructor(type, props, children) {
        this.type     = type;      // "View" | "Text" | …
        this.props    = props || {};
        this.children = children || [];
        this.native   = null;      // int handle (LVGL pointer)
    }
}

/* ── Renderer ───────────────────────────────────────────────────────────── */

/**
 * bindProp — apply a single property to a native node.
 *
 * If the value is a function it is treated as a reactive accessor: an effect is
 * created so that only this property is re-applied when its signals change.
 */
function bindProp(native, key, value) {
    if (typeof value === "function") {
        createEffect(() => lv.setProperty(native, key, value()));
    } else {
        lv.setProperty(native, key, value);
    }
}

function applyProp(native, key, value) {
    if (key === "children") return;

    /* Style object: bind each entry (each may itself be reactive) */
    if (key === "style") {
        if (value) {
            for (const [sk, sv] of Object.entries(value)) bindProp(native, sk, sv);
        }
        return;
    }

    /* Event handlers: onClick → "click", onLongPress → "longpress" */
    if (key.startsWith("on") && typeof value === "function") {
        const eventName = key[2].toLowerCase() + key.slice(3).toLowerCase();
        lv.addEvent(native, eventName, value);
        return;
    }

    bindProp(native, key, value);
}

function mountVNode(vnode, parentNative) {
    const native = lv.createNode(vnode.type);
    vnode.native = native;

    if (parentNative !== undefined)
        lv.appendChild(parentNative, native);

    for (const [k, v] of Object.entries(vnode.props)) applyProp(native, k, v);

    for (const child of vnode.children) {
        if (child instanceof VNode) {
            mountVNode(child, native);
        } else if (typeof child === "function") {
            /* Reactive text child */
            createEffect(() => lv.setProperty(native, "text", String(child())));
        } else if (child !== null && child !== undefined) {
            /* Bare string/number → text shorthand on this node */
            lv.setProperty(native, "text", String(child));
        }
    }

    return native;
}

/* ── JSX factory ────────────────────────────────────────────────────────── */

export function h(type, props, ...children) {
    /* Flatten nested arrays (JSX maps) */
    const flat = children.flat(Infinity);
    return new VNode(type, props, flat);
}

/* ── Mount ──────────────────────────────────────────────────────────────── */

/**
 * render(fn, containerNative?)
 *
 * fn is a zero-arg function returning a VNode (or array of VNodes). The tree is
 * mounted ONCE; reactive props keep themselves up to date via fine-grained
 * effects. There is no full-tree remount on signal changes.
 */
export function render(fn, containerNative) {
    const root = containerNative !== undefined
        ? containerNative
        : lv.getScreen();

    const tree  = fn();
    const nodes = Array.isArray(tree) ? tree : [tree];
    for (const vnode of nodes) {
        if (vnode instanceof VNode) mountVNode(vnode, root);
    }
}
