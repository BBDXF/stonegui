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
function Row({ height = 56, gap = 12, children }) {
  return /* @__PURE__ */ h("view", { style: {
    flexFlow: "row",
    width: "100%",
    height,
    gap,
    alignItems: "center"
  } }, children);
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
  return /* @__PURE__ */ h("text", { style: { textColor: color, width } }, () => `${label}: ${value()}`);
}
const FEATURES = ["LVGL 9", "QuickJS", "SDL2", "JSX"];
function App() {
  const [count, setCount] = createSignal(0);
  const [progress, setProgress] = createSignal(20);
  const [on, setOn] = createSignal(false);
  const [volume, setVolume] = createSignal(40);
  const [agree, setAgree] = createSignal(false);
  const [fruit, setFruit] = createSignal(0);
  const FRUITS = ["\u82F9\u679C", "\u9999\u8549", "\u6A59\u5B50"];
  return /* @__PURE__ */ h(
    "view",
    {
      style: {
        width: "100%",
        height: "100%",
        backgroundColor: "#1e1e2e",
        padding: 20,
        flexFlow: "column",
        gap: 12
      }
    },
    /* @__PURE__ */ h("text", { style: { textColor: "white", font: fontTitle } }, "JSX \u6F14\u793A / Demo"),
    /* @__PURE__ */ h(Row, null, /* @__PURE__ */ h(Stat, { color: "#a6e3a1cc", label: "\u8BA1\u6570", value: count }), /* @__PURE__ */ h(PillButton, { color: "blue", onClick: () => {
      setCount((c) => c + 1);
      setProgress((c) => c + 1);
    } }, "\u52A0\u4E00"), /* @__PURE__ */ h(
      PillButton,
      {
        color: "pink",
        onClick: () => {
          setCount(0);
          setProgress(0);
        }
      },
      "\u91CD\u7F6E"
    )),
    /* @__PURE__ */ h(Row, null, /* @__PURE__ */ h(
      Stat,
      {
        color: "#cba6f7aa",
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
    ), /* @__PURE__ */ h(
      PillButton,
      {
        color: "lime",
        width: 110,
        onClick: () => setProgress((p) => Math.min(100, p + 10))
      },
      "+10%"
    )),
    /* @__PURE__ */ h(Row, { height: 48 }, /* @__PURE__ */ h(
      Stat,
      {
        color: "orange",
        label: "\u97F3\u91CF",
        value: () => `${volume()}`,
        width: 200
      }
    ), /* @__PURE__ */ h(
      "slider",
      {
        style: { flexGrow: 1, height: 10 },
        min: 0,
        max: 100,
        value: () => volume(),
        onChange: (v) => setVolume(v)
      }
    )),
    /* @__PURE__ */ h(Row, { height: 48 }, /* @__PURE__ */ h(
      Stat,
      {
        color: "#f9e2af30",
        label: "\u5F00\u5173",
        value: () => on() ? "\u5F00" : "\u5173",
        width: 200
      }
    ), /* @__PURE__ */ h("switch", { checked: () => on(), onChange: (v) => setOn(v) })),
    /* @__PURE__ */ h(Row, { height: 56 }, /* @__PURE__ */ h(
      "checkbox",
      {
        text: "\u540C\u610F\u6761\u6B3E",
        style: { textColor: "silver", width: 200 },
        checked: () => agree(),
        onChange: (v) => setAgree(v)
      }
    ), /* @__PURE__ */ h(
      "dropdown",
      {
        style: { width: 160 },
        options: FRUITS.join("\n"),
        value: () => fruit(),
        onChange: (i) => setFruit(i)
      }
    ), /* @__PURE__ */ h("text", { style: { textColor: "#bac2dedd", width: 120 } }, () => `\u9009\u62E9: ${FRUITS[fruit()]}`), /* @__PURE__ */ h("spinner", { style: { width: 36, height: 36 }, spinTime: 2800 })),
    /* @__PURE__ */ h(Row, { height: 80 }, /* @__PURE__ */ h(
      Stat,
      {
        color: "#94e2d5",
        label: "\u5706\u5F27",
        value: () => `${progress()}%`,
        width: 200
      }
    ), /* @__PURE__ */ h(
      "arc",
      {
        style: { width: 72, height: 72 },
        min: 0,
        max: 100,
        value: () => progress()
      }
    )),
    /* @__PURE__ */ h(Fragment, null, /* @__PURE__ */ h("text", { style: { textColor: "cyan", width: "100%", height: 32 } }, "\u6280\u672F\u6808 / Stack:"), /* @__PURE__ */ h(Row, { height: 40 }, FEATURES.map((name) => /* @__PURE__ */ h("text", { style: { textColor: "gray", width: 130 } }, `\u2022 ${name}`))))
  );
}
render(() => /* @__PURE__ */ h(App, null));
