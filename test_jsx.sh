#!/usr/bin/env bash
# test_jsx.sh — 构建 stonegui、编译 JSX demo 并运行
#
# 用法：
#   ./test_jsx.sh            # cmake构建 + 编译JSX + 运行（默认）
#   ./test_jsx.sh --build    # cmake构建 + 编译JSX，不运行
#   ./test_jsx.sh --run      # 只运行（跳过所有编译）
#   ./test_jsx.sh --no-cmake # 跳过cmake，只编译JSX并运行

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
JSX_DIR="$SCRIPT_DIR/examples/jsx"
BUILD_DIR="$SCRIPT_DIR/build"
BINARY="$BUILD_DIR/stonegui"
APP_JS="$JSX_DIR/app.js"

BUILD=1
RUN=1
CMAKE=1

for arg in "$@"; do
    case "$arg" in
        --build)    RUN=0    ;;
        --run)      BUILD=0; CMAKE=0 ;;
        --no-cmake) CMAKE=0  ;;
    esac
done

# ── 1. cmake 构建 stonegui ────────────────────────────────────────────────
if [[ $CMAKE -eq 1 ]]; then
    echo ">>> 配置 cmake（build/）..."
    cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR" 
    # cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release


    JOBS=$(nproc 2>/dev/null || echo 4)
    echo ">>> make -j$JOBS ..."
    cmake --build "$BUILD_DIR" -- -j"$JOBS"

    echo ">>> stonegui 构建完成：$BINARY"
fi

# ── 2. 编译 JSX → JS ──────────────────────────────────────────────────────
if [[ $BUILD -eq 1 ]]; then
    echo ">>> 安装依赖（如已安装则跳过）..."
    cd "$JSX_DIR"
    npm install --prefer-offline --no-audit --no-fund

    echo ">>> 编译 app.jsx → app.js ..."
    npm run build

    echo ">>> 编译完成：$APP_JS"
    cd "$SCRIPT_DIR"
fi

# ── 3. 运行 stonegui ──────────────────────────────────────────────────────
if [[ $RUN -eq 1 ]]; then
    if [[ ! -f "$BINARY" ]]; then
        echo "错误：找不到可执行文件 $BINARY" >&2
        echo "请先执行：cmake --build build/" >&2
        exit 1
    fi
    if [[ ! -f "$APP_JS" ]]; then
        echo "错误：找不到 $APP_JS，请先执行编译步骤" >&2
        exit 1
    fi

    echo ">>> 运行：$BINARY $APP_JS"
    exec "$BINARY" "$APP_JS"
fi
