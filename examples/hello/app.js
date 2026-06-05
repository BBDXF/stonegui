/**
 * app.js — stonegui MVP demo
 *
 * Demonstrates:
 *  - View, Text, Button, Progress, Switch, Input widgets
 *  - Signals with fine-grained updates (reactive accessor thunks)
 *  - Responsive layout (percent sizes that follow window resize)
 *  - Chinese text via a TTF font loaded with lv.loadFont()
 *  - Keyboard input into the Input field (click to focus, then type)
 */

import { h, render, createSignal, loadFont } from "../../js/framework.js";

/* ── Fonts ──
 * Load a CJK-capable TTF/TTC so Chinese renders. We try a few common system
 * paths; loadFont() returns 0 on failure, in which case we fall back to the
 * built-in font (Latin only).
 */
const FONT_CANDIDATES = [
    "/usr/share/fonts/truetype/wqy/wqy-microhei.ttc",
    "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
    "/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc",
    "/System/Library/Fonts/PingFang.ttc",
];

function loadCjk(size) {
    for (const p of FONT_CANDIDATES) {
        const f = loadFont(p, size);
        if (f) return f;
    }
    return 0;
}

const fontTitle = loadCjk(28);
const fontBody  = loadCjk(20);

/* Apply a loaded font only if available (0 = use built-in) */
const withFont = (font, style) => (font ? { ...style, font } : style);

/* ── State ── */
const [count, setCount]       = createSignal(0);
const [progress, setProgress] = createSignal(20);
const [on, setOn]             = createSignal(false);

/* ── App component ── */
function App() {
    return h("View", {
        /* Root fills the screen and reflows on resize */
        style: {
            width: "100%",
            height: "100%",
            backgroundColor: "#1e1e2e",
            padding: 20,
            flexFlow: "column",
            scrollable: false,
        }
    },
        /* Title */
        h("Text", {
            style: withFont(fontTitle, {
                textColor: "#cdd6f4",
                fontSize: 24,
            }),
            text: "stonegui 演示 / MVP",
        }),

        h("View", { style: { height: 16, width: "100%" } }),

        /* Counter row */
        h("View", {
            style: { flexFlow: "row", width: "100%", height: 60, scrollable: false }
        },
            h("Text", {
                style: withFont(fontBody, { textColor: "#a6e3a1", width: 280, height: 50 }),
                text: () => `计数: ${count()}`,
            }),
            h("Button", {
                style: withFont(fontBody, {
                    width: 110, height: 44, backgroundColor: "#89b4fa", borderRadius: 8,
                }),
                text: "加一",
                onClick: () => setCount(c => c + 1),
            }),
            h("View", { style: { width: 12, height: 44 } }),
            h("Button", {
                style: withFont(fontBody, {
                    width: 110, height: 44, backgroundColor: "#f38ba8", borderRadius: 8,
                }),
                text: "重置",
                onClick: () => { setCount(0); setProgress(20); },
            }),
        ),

        h("View", { style: { height: 16, width: "100%" } }),

        /* Progress row */
        h("View", {
            style: { flexFlow: "row", width: "100%", height: 60, scrollable: false }
        },
            h("Text", {
                style: withFont(fontBody, { textColor: "#cba6f7", width: 200, height: 50 }),
                text: () => `进度: ${progress()}%`,
            }),
            h("Progress", {
                style: { flexGrow: 1, height: 24, borderRadius: 12 },
                min: 0, max: 100,
                value: () => progress(),
            }),
            h("View", { style: { width: 12 } }),
            h("Button", {
                style: withFont(fontBody, {
                    width: 110, height: 44, backgroundColor: "#a6e3a1", borderRadius: 8,
                }),
                text: "+10%",
                onClick: () => setProgress(p => Math.min(100, p + 10)),
            }),
        ),

        h("View", { style: { height: 16, width: "100%" } }),

        /* Switch row */
        h("View", {
            style: { flexFlow: "row", width: "100%", height: 50, scrollable: false }
        },
            h("Text", {
                style: withFont(fontBody, { textColor: "#f9e2af", width: 200, height: 40 }),
                text: () => `开关: ${on() ? "开" : "关"}`,
            }),
            h("Switch", {
                checked: () => on(),
                onChange: () => setOn(v => !v),
            }),
        ),

        h("View", { style: { height: 16, width: "100%" } }),

        /* Input row — click to focus, then type (keyboard) */
        h("Input", {
            style: withFont(fontBody, {
                width: "100%",
                height: 44,
                backgroundColor: "#313244",
                textColor: "#cdd6f4",
                borderRadius: 8,
                borderWidth: 1,
                borderColor: "#585b70",
            }),
            placeholder: "点击此处输入文字…",
        }),
    );
}

/* ── Mount ── */
render(App);
