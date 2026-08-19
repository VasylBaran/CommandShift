#!/bin/bash
# Build, sign and install CommandShift locally.
#
#   ./build.sh          build + sign
#   ./build.sh install  build + sign + replace /Applications/CommandShift.app
#
# Requires: brew install qt   and a "CommandShift Dev" code-signing identity
# (see README "Building from source").

set -euo pipefail

QT_PREFIX="${QT_PREFIX:-/opt/homebrew/opt/qt}"
QMAKE="$QT_PREFIX/bin/qmake"
MACDEPLOYQT="$QT_PREFIX/bin/macdeployqt"
IDENTITY="${CS_SIGN_IDENTITY:-CommandShift Dev}"

ROOT="$(cd "$(dirname "$0")" && pwd)"
BUILD="$ROOT/build"
APP="$BUILD/CommandShift.app"

[ -x "$QMAKE" ] || { echo "qmake not found at $QMAKE -- run: brew install qt"; exit 1; }

echo "==> Configuring"
mkdir -p "$BUILD"
cd "$BUILD"
"$QMAKE" "$ROOT/src/CommandShift.pro" CONFIG+=release

echo "==> Building"
make -j"$(sysctl -n hw.ncpu)"

echo "==> Bundling Qt frameworks"
"$MACDEPLOYQT" "$APP"

echo "==> Signing as '$IDENTITY'"
# Sign nested code first, then the bundle, so seals stay valid.
find "$APP/Contents/Frameworks" "$APP/Contents/PlugIns" -name '*.dylib' -o -name '*.framework' 2>/dev/null \
  | while read -r item; do codesign --force --timestamp=none --sign "$IDENTITY" "$item" >/dev/null 2>&1 || true; done
codesign --force --deep --timestamp=none --sign "$IDENTITY" "$APP"
codesign --verify --verbose=2 "$APP"

echo "==> Built: $APP"

if [ "${1:-}" = "install" ]; then
    echo "==> Installing to /Applications"
    pkill -x CommandShift 2>/dev/null || true
    sleep 1
    rm -rf /Applications/CommandShift.app
    cp -R "$APP" /Applications/CommandShift.app
    echo "==> Installed. Launching."
    open /Applications/CommandShift.app
fi
