/**
 * framework.js — Minimal reactive UI framework for stonegui
 *
 * Provides:
 *   createSignal(init)         → [get, set]
 *   createEffect(fn)           → auto-tracks signals used inside fn
 *   h(type, props, ...children) → creates/updates LVGL nodes
 *   render(rootFn, container)  → mounts a component tree
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

/* ── Node pool ──────────────────────────────────────────────────────────── */

/* A VNode describes one piece of UI */
class VNode {
    constructor(type, props, children) {
        this.type     = type;      // "View" | "Text" | …
        this.props    = props || {};
        this.children = children || [];
        this.native   = null;      // int handle (LVGL pointer)
    }
}

/* ── Renderer ───────────────────────────────────────────────────────────── */

function applyProps(native, props) {
    if (!props) return;

    /* Flatten style object into top-level calls */
    if (props.style) {
        for (const [k, v] of Object.entries(props.style)) {
            lv.setProperty(native, k, v);
        }
    }

    for (const [k, v] of Object.entries(props)) {
        if (k === "style" || k === "children") continue;

        /* Event handlers: onClick → "click", onLongPress → "longpress" */
        if (k.startsWith("on") && typeof v === "function") {
            const eventName = k[2].toLowerCase() + k.slice(3).toLowerCase();
            lv.addEvent(native, eventName, v);
            continue;
        }

        lv.setProperty(native, k, v);
    }
}

function mountVNode(vnode, parentNative) {
    const native = lv.createNode(vnode.type);
    vnode.native = native;

    if (parentNative !== undefined)
        lv.appendChild(parentNative, native);

    applyProps(native, vnode.props);

    for (const child of vnode.children) {
        if (typeof child === "string") {
            /* Treat bare string as text shorthand on the parent node */
            lv.setProperty(native, "text", child);
        } else if (child instanceof VNode) {
            mountVNode(child, native);
        }
    }
}

/* ── JSX factory ────────────────────────────────────────────────────────── */

export function h(type, props, ...children) {
    /* Flatten nested arrays (JSX maps) */
    const flat = children.flat(Infinity);
    return new VNode(type, props, flat);
}

/* ── Reactive render ────────────────────────────────────────────────────── */

/**
 * render(fn, containerNative?)
 *
 * fn is a zero-arg function that returns a VNode (the component tree).
 * Every time a signal read inside fn changes, the tree is rebuilt.
 *
 * For the MVP we do a simple full-remount on change.
 * (A production renderer would diff and patch.)
 */
export function render(fn, containerNative) {
    const root = containerNative !== undefined
        ? containerNative
        : lv.getScreen();

    let prevNodes = [];

    createEffect(() => {
        const tree = fn();

        /* Remove previous nodes */
        for (const n of prevNodes) lv.dispose(n);
        prevNodes = [];

        const nodes = Array.isArray(tree) ? tree : [tree];
        for (const vnode of nodes) {
            if (vnode instanceof VNode) {
                mountVNode(vnode, root);
                prevNodes.push(vnode.native);
            }
        }
    });
}
