#!/usr/bin/env bash
# Generate mac/installer/HackPass.icns from shared/assets/logo.svg.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

SVG="$REPO_ROOT/shared/assets/logo.svg"
OUT="$REPO_ROOT/mac/installer/HackPass.icns"

if [[ ! -f "$SVG" ]]; then
    echo "logo.svg not found at $SVG" >&2
    exit 1
fi

# rsvg-convert (brew install librsvg) preferred; qlmanage fallback.
have_rsvg() { command -v rsvg-convert >/dev/null; }

ICONSET="$(mktemp -d)/HackPass.iconset"
mkdir -p "$ICONSET"

render_png() {
    local size="$1" out="$2"
    if have_rsvg; then
        rsvg-convert -w "$size" -h "$size" "$SVG" -o "$out"
    else
        qlmanage -t -s "$size" -o "$(dirname "$out")" "$SVG" >/dev/null 2>&1
        mv "$(dirname "$out")/$(basename "$SVG").png" "$out"
    fi
}

for s in 16 32 64 128 256 512; do
    render_png "$s"        "$ICONSET/icon_${s}x${s}.png"
    render_png "$((s * 2))" "$ICONSET/icon_${s}x${s}@2x.png"
done

mkdir -p "$(dirname "$OUT")"
iconutil -c icns -o "$OUT" "$ICONSET"
rm -rf "$(dirname "$ICONSET")"

echo "Wrote $OUT  ($(stat -f%z "$OUT") bytes)"
