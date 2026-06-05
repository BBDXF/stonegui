/**
 * app.jsx — stonegui JSX (React/Vue-style) demo
 *
 * Written in real JSX and transpiled to plain JS with esbuild:
 *
 *     cd examples/jsx
 *     npm install      # once, installs esbuild
 *     npm run build    # app.jsx → app.js
 *
 * Run it (from the repo root):
 *
 *     ./build/stonegui examples/jsx/app.js
 *
 * Demonstrates a React/Vue-like authoring style on top of stonegui:
 *   - JSX elements & nesting
 *   - Function components with props and `children`
 *   - Fragments (<>…</>)
 *   - List rendering with .map()
 *   - Signals + events with fine-grained, property-only updates
 *   - Chinese text via a global default font
 *
 * Conventions (like React DOM):
 *   - lowercase tags  → host widgets:  view, text, button, switch, progress,
 *                                       input, image
 *   - Capitalized tags → components (functions)
 *
 * esbuild is configured (see package.json) with `h` as the JSX factory and
 * `Fragment` as the fragment factory, so we import both from the framework.
 */

import {
    h, Fragment, render, createSignal, loadFont, setDefaultFont,
} from "../../js/framework.js";

/* ── Fonts ── */
const FONT_CANDIDATES = [
    "/usr/share/fonts/truetype/wqy/wqy-microhei.ttc",
    "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
    "/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc",
    "/System/Library/Fonts/PingFang.ttc",
];
const loadCjk = (size) => {
    for (const p of FONT_CANDIDATES) {
        const f = loadFont(p, size);
        if (f) return f;
    }
    return 0;
};
const fontTitle = loadCjk(30);
setDefaultFont(loadCjk(20));

/* ── Reusable components ─────────────────────────────────────────────────── */

/* A horizontal row that lays its children out left-to-right. */
function Row({ height = 56, children }) {
    return (
        <view style={{ flexFlow: "row", width: "100%", height }}>
            {children}
        </view>
    );
}

/* A coloured pill button; children are the label. */
function PillButton({ color, onClick, width = 120, children }) {
    return (
        <button
            style={{ width, height: 44, backgroundColor: color, borderRadius: 8 }}
            onClick={onClick}
        >
            {children}
        </button>
    );
}

/* A labelled, reactive value, e.g.  计数: 3
 * `value` is an accessor (signal getter or thunk) so updates are property-only. */
function Stat({ color, label, value, width = 240 }) {
    return (
        <text style={{ textColor: color, width, height: 40 }}>
            {() => `${label}: ${value()}`}
        </text>
    );
}

/* ── Application ─────────────────────────────────────────────────────────── */

const FEATURES = ["LVGL 9", "QuickJS", "SDL2", "JSX"];

function App() {
    const [count, setCount]       = createSignal(0);
    const [progress, setProgress] = createSignal(20);
    const [on, setOn]             = createSignal(false);
    const [volume, setVolume]     = createSignal(40);
    const [agree, setAgree]       = createSignal(false);
    const [fruit, setFruit]       = createSignal(0);

    const FRUITS = ["苹果", "香蕉", "橙子"];

    return (
        <view
            style={{
                width: "100%",
                height: "100%",
                backgroundColor: "#1e1e2e",
                padding: 20,
                flexFlow: "column",
            }}
        >
            <text style={{ textColor: "#cdd6f4", font: fontTitle }}>
                JSX 演示 / Demo
            </text>

            <view style={{ height: 12, width: "100%" }} />

            {/* Counter */}
            <Row>
                <Stat color="#a6e3a1" label="计数" value={count} />
                <PillButton color="#89b4fa" onClick={() => setCount((c) => c + 1)}>
                    加一
                </PillButton>
                <view style={{ width: 12 }} />
                <PillButton color="#f38ba8"
                            onClick={() => { setCount(0); setProgress(20); }}>
                    重置
                </PillButton>
            </Row>

            <view style={{ height: 12, width: "100%" }} />

            {/* Progress */}
            <Row>
                <Stat color="#cba6f7" label="进度"
                      value={() => `${progress()}%`} width={200} />
                <progress style={{ flexGrow: 1, height: 24, borderRadius: 12 }}
                          min={0} max={100} value={() => progress()} />
                <view style={{ width: 12 }} />
                <PillButton color="#a6e3a1" width={110}
                            onClick={() => setProgress((p) => Math.min(100, p + 10))}>
                    +10%
                </PillButton>
            </Row>

            <view style={{ height: 12, width: "100%" }} />

            {/* Slider — onChange receives the slider's value */}
            <Row height={48}>
                <Stat color="#fab387" label="音量"
                      value={() => `${volume()}`} width={200} />
                <slider style={{ flexGrow: 1, height: 10 }}
                        min={0} max={100} value={() => volume()}
                        onChange={(v) => setVolume(v)} />
            </Row>

            <view style={{ height: 12, width: "100%" }} />

            {/* Switch */}
            <Row height={48}>
                <Stat color="#f9e2af" label="开关"
                      value={() => (on() ? "开" : "关")} width={200} />
                <switch checked={() => on()} onChange={(v) => setOn(v)} />
            </Row>

            <view style={{ height: 12, width: "100%" }} />

            {/* Checkbox + Dropdown + Spinner */}
            <Row height={56}>
                <checkbox text="同意条款"
                          style={{ textColor: "#cdd6f4", width: 200 }}
                          checked={() => agree()}
                          onChange={(v) => setAgree(v)} />
                <dropdown style={{ width: 160 }}
                          options={FRUITS.join("\n")}
                          value={() => fruit()}
                          onChange={(i) => setFruit(i)} />
                <view style={{ width: 16 }} />
                <text style={{ textColor: "#bac2de", width: 120, height: 32 }}>
                    {() => `选择: ${FRUITS[fruit()]}`}
                </text>
                <spinner style={{ width: 36, height: 36 }} />
            </Row>

            <view style={{ height: 12, width: "100%" }} />

            {/* List rendering with .map() wrapped in a Fragment */}
            <>
                <text style={{ textColor: "#94e2d5", width: "100%", height: 32 }}>
                    技术栈 / Stack:
                </text>
                <Row height={40}>
                    {FEATURES.map((name) => (
                        <text style={{ textColor: "#bac2de", width: 130, height: 32 }}>
                            {`• ${name}`}
                        </text>
                    ))}
                </Row>
            </>
        </view>
    );
}

/* ── Mount ── */
render(() => <App />);
