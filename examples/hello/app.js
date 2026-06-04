/**
 * app.js — stonegui MVP demo
 *
 * Demonstrates:
 *  - View, Text, Button, Progress widgets
 *  - Signals (reactive state)
 *  - Style props
 *  - Click events
 */

import { h, render, createSignal, createEffect } from "../../js/framework.js";

/* ── State ── */
const [count, setCount] = createSignal(0);
const [progress, setProgress] = createSignal(20);

/* ── App component ── */
function App() {
    return h("View", {
        style: {
            width: 780,
            height: 460,
            x: 10,
            y: 10,
            backgroundColor: "#1e1e2e",
            borderRadius: 12,
            padding: 20,
            flexFlow: "column",
        }
    },
        /* Title */
        h("Text", {
            style: {
                textColor: "#cdd6f4",
                fontSize: 24,
            },
            text: "stonegui MVP",
        }),

        /* Spacer */
        h("View", { style: { height: 20, width: 740 } }),

        /* Counter row */
        h("View", {
            style: {
                flexFlow: "row",
                width: 740,
                height: 60,
            }
        },
            h("Text", {
                style: { textColor: "#a6e3a1", width: 300, height: 50 },
                text: `Count: ${count()}`,
            }),
            h("Button", {
                style: {
                    width: 120,
                    height: 44,
                    backgroundColor: "#89b4fa",
                    borderRadius: 8,
                },
                text: "+1",
                onClick: () => setCount(c => c + 1),
            }),
            h("View", { style: { width: 16, height: 44 } }),
            h("Button", {
                style: {
                    width: 120,
                    height: 44,
                    backgroundColor: "#f38ba8",
                    borderRadius: 8,
                },
                text: "Reset",
                onClick: () => { setCount(0); setProgress(20); },
            }),
        ),

        /* Spacer */
        h("View", { style: { height: 20, width: 740 } }),

        /* Progress row */
        h("View", {
            style: { flexFlow: "row", width: 740, height: 60 }
        },
            h("Text", {
                style: { textColor: "#cba6f7", width: 220, height: 50 },
                text: `Progress: ${progress()}%`,
            }),
            h("Progress", {
                style: { width: 340, height: 24, borderRadius: 12 },
                min: 0,
                max: 100,
                value: progress(),
            }),
            h("View", { style: { width: 16 } }),
            h("Button", {
                style: {
                    width: 120,
                    height: 44,
                    backgroundColor: "#a6e3a1",
                    borderRadius: 8,
                },
                text: "+10%",
                onClick: () => setProgress(p => Math.min(100, p + 10)),
            }),
        ),

        /* Spacer */
        h("View", { style: { height: 20, width: 740 } }),

        /* Input row */
        h("Input", {
            style: {
                width: 400,
                height: 44,
                backgroundColor: "#313244",
                borderRadius: 8,
                borderWidth: 1,
                borderColor: "#585b70",
            },
            placeholder: "Type something…",
        }),
    );
}

/* ── Mount ── */
render(App);
