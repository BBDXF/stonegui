/**
 * app.js — stonegui MVP demo
 *
 * Demonstrates:
 *  - View, Text, Button, Progress, Switch, Input widgets
 *  - Signals with fine-grained updates (reactive accessor thunks)
 *  - Responsive layout (percent sizes that follow window resize)
 *  - A GLOBAL default font (CJK), with a per-element font override for the title
 *  - Keyboard input into the Input field (click to focus, then type)
 */

import {
    h, render, createSignal, loadFont, setDefaultFont,
} from "../../js/framework.js";

/* ── Fonts ──
 * Tiny-TTF fonts are a fixed pixel size per handle, so we load one handle per
 * size we need. The body size becomes the GLOBAL default; the title uses a
 * larger handle as an explicit override (style.font).
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

const fontDefault = loadCjk(20);
const fontTitle   = loadCjk(30);

/* Make the CJK font the global default — every Text/Button inherits it. */
setDefaultFont(fontDefault);

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
        }
    },
        /* Title — overrides the default font with a larger one */
        h("Text", {
            style: { textColor: "#cdd6f4", font: fontTitle },
            text: "stonegui 演示 / MVP",
        }),

        h("View", { style: { height: 16, width: "100%" } }),

        /* Counter row */
        h("View", {
            style: { flexFlow: "row", width: "100%", height: 60 }
        },
            h("Text", {
                style: { textColor: "#a6e3a1", width: 280, height: 50 },
                text: () => `计数: ${count()}`,
            }),
            h("Button", {
                style: {
                    width: 110, height: 44, backgroundColor: "#89b4fa", borderRadius: 8,
                },
                text: "加一",
                onClick: () => setCount(c => c + 1),
            }),
            h("View", { style: { width: 12, height: 44 } }),
            h("Button", {
                style: {
                    width: 110, height: 44, backgroundColor: "#f38ba8", borderRadius: 8,
                },
                text: "重置",
                onClick: () => { setCount(0); setProgress(20); },
            }),
        ),

        h("View", { style: { height: 16, width: "100%" } }),

        /* Progress row */
        h("View", {
            style: { flexFlow: "row", width: "100%", height: 60 }
        },
            h("Text", {
                style: { textColor: "#cba6f7", width: 200, height: 50 },
                text: () => `进度: ${progress()}%`,
            }),
            h("Progress", {
                style: { flexGrow: 1, height: 24, borderRadius: 12 },
                min: 0, max: 100,
                value: () => progress(),
            }),
            h("View", { style: { width: 12 } }),
            h("Button", {
                style: {
                    width: 110, height: 44, backgroundColor: "#a6e3a1", borderRadius: 8,
                },
                text: "+10%",
                onClick: () => setProgress(p => Math.min(100, p + 10)),
            }),
        ),

        h("View", { style: { height: 16, width: "100%" } }),

        /* Switch row */
        h("View", {
            style: { flexFlow: "row", width: "100%", height: 50 }
        },
            h("Text", {
                style: { textColor: "#f9e2af", width: 200, height: 40 },
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
            style: {
                width: "100%",
                height: 44,
                backgroundColor: "#313244",
                textColor: "#cdd6f4",
                borderRadius: 8,
                borderWidth: 1,
                borderColor: "#585b70",
            },
            placeholder: "点击此处输入文字…",
        }),
    );
}

/* ── Mount ── */
render(App);
