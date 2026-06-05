import {
  h,
  Fragment,
  render,
  createSignal,
  loadFont,
  setDefaultFont
} from "../../js/framework.js";
const FONT_CANDIDATES = [
  "/usr/share/fonts/truetype/wqy/wqy-microhei.ttc",
  "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
  "/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc",
  "/System/Library/Fonts/PingFang.ttc"
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
function Row({ height = 56, children }) {
  return /* @__PURE__ */ h("view", { style: { flexFlow: "row", width: "100%", height } }, children);
}
function PillButton({ color, onClick, width = 120, children }) {
  return /* @__PURE__ */ h(
    "button",
    {
      style: { width, height: 44, backgroundColor: color, borderRadius: 8 },
      onClick
    },
    children
  );
}
function Stat({ color, label, value, width = 240 }) {
  return /* @__PURE__ */ h("text", { style: { textColor: color, width, height: 40 } }, () => `${label}: ${value()}`);
}
const FEATURES = ["LVGL 9", "QuickJS", "SDL2", "JSX"];
function App() {
  const [count, setCount] = createSignal(0);
  const [progress, setProgress] = createSignal(20);
  const [on, setOn] = createSignal(false);
  return /* @__PURE__ */ h(
    "view",
    {
      style: {
        width: "100%",
        height: "100%",
        backgroundColor: "#1e1e2e",
        padding: 20,
        flexFlow: "column"
      }
    },
    /* @__PURE__ */ h("text", { style: { textColor: "#cdd6f4", font: fontTitle } }, "JSX \u6F14\u793A / Demo"),
    /* @__PURE__ */ h("view", { style: { height: 12, width: "100%" } }),
    /* @__PURE__ */ h(Row, null, /* @__PURE__ */ h(Stat, { color: "#a6e3a1", label: "\u8BA1\u6570", value: count }), /* @__PURE__ */ h(PillButton, { color: "#89b4fa", onClick: () => setCount((c) => c + 1) }, "\u52A0\u4E00"), /* @__PURE__ */ h("view", { style: { width: 12 } }), /* @__PURE__ */ h(
      PillButton,
      {
        color: "#f38ba8",
        onClick: () => {
          setCount(0);
          setProgress(20);
        }
      },
      "\u91CD\u7F6E"
    )),
    /* @__PURE__ */ h("view", { style: { height: 12, width: "100%" } }),
    /* @__PURE__ */ h(Row, null, /* @__PURE__ */ h(
      Stat,
      {
        color: "#cba6f7",
        label: "\u8FDB\u5EA6",
        value: () => `${progress()}%`,
        width: 200
      }
    ), /* @__PURE__ */ h(
      "progress",
      {
        style: { flexGrow: 1, height: 24, borderRadius: 12 },
        min: 0,
        max: 100,
        value: () => progress()
      }
    ), /* @__PURE__ */ h("view", { style: { width: 12 } }), /* @__PURE__ */ h(
      PillButton,
      {
        color: "#a6e3a1",
        width: 110,
        onClick: () => setProgress((p) => Math.min(100, p + 10))
      },
      "+10%"
    )),
    /* @__PURE__ */ h("view", { style: { height: 12, width: "100%" } }),
    /* @__PURE__ */ h(Row, { height: 48 }, /* @__PURE__ */ h(
      Stat,
      {
        color: "#f9e2af",
        label: "\u5F00\u5173",
        value: () => on() ? "\u5F00" : "\u5173",
        width: 200
      }
    ), /* @__PURE__ */ h("switch", { checked: () => on(), onChange: () => setOn((v) => !v) })),
    /* @__PURE__ */ h("view", { style: { height: 12, width: "100%" } }),
    /* @__PURE__ */ h(Fragment, null, /* @__PURE__ */ h("text", { style: { textColor: "#94e2d5", width: "100%", height: 32 } }, "\u6280\u672F\u6808 / Stack:"), /* @__PURE__ */ h(Row, { height: 40 }, FEATURES.map((name) => /* @__PURE__ */ h("text", { style: { textColor: "#bac2de", width: 130, height: 32 } }, `\u2022 ${name}`))))
  );
}
render(() => /* @__PURE__ */ h(App, null));
