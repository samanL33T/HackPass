#!/usr/bin/env bash
# Build HackPass.app, stage the compiled backend + openssl + extension,
# ad-hoc sign, package as DMG. Output: mac/release/.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

step()  { printf "\n==> %s\n" "$1"; }
ok()    { printf "    ok: %s\n" "$1"; }
warn()  { printf "    warn: %s\n" "$1"; }

bash "$SCRIPT_DIR/setup.sh" --skip-backend

APP_BUNDLE="$(find "$REPO_ROOT/build" -name HackPass.app -type d | head -n1)"
if [[ -z "$APP_BUNDLE" ]]; then echo "no HackPass.app"; exit 1; fi
ok "App at $APP_BUNDLE"

if ! command -v bun >/dev/null; then
    echo "    error: bun missing; install via brew install bun."
    exit 1
fi
step "Compiling backend (bun --target=bun-darwin-arm64)"
BACKEND_OUT="$APP_BUNDLE/Contents/Resources/hackpass-server"
(cd "$REPO_ROOT/shared/backend" \
    && [[ -d node_modules ]] || bun install --silent \
    && bun build src/server.ts --compile --minify --sourcemap=none \
       --outfile "$BACKEND_OUT" --target=bun-darwin-arm64)
ok "hackpass-server staged inside the bundle ($(du -sh "$BACKEND_OUT" | cut -f1))"

# Bundle homebrew openssl@3 (system openssl is LibreSSL).
step "Staging openssl binary"
OPENSSL_BIN=""
for candidate in \
    "${OPENSSL_ROOT_DIR:-}/bin/openssl" \
    "/opt/homebrew/opt/openssl@3/bin/openssl" \
    "/usr/local/opt/openssl@3/bin/openssl"
do
    if [[ -x "$candidate" ]]; then
        OPENSSL_BIN="$candidate"
        break
    fi
done
if [[ -n "$OPENSSL_BIN" ]]; then
    cp "$OPENSSL_BIN" "$APP_BUNDLE/Contents/Resources/openssl"
    chmod +x "$APP_BUNDLE/Contents/Resources/openssl"
    ok "openssl staged from $OPENSSL_BIN"
else
    warn "No homebrew openssl@3 found - the backend will fall back to /usr/bin/openssl (LibreSSL), which may not support ec_paramgen_curve syntax"
fi

# Stage the extension inside the bundle as a fallback copy.
step "Staging extension"
EXT_OUT="$APP_BUNDLE/Contents/Resources/extension"
rm -rf "$EXT_OUT"
cp -R "$REPO_ROOT/shared/extension" "$EXT_OUT"
find "$EXT_OUT" -name node_modules -type d -prune -exec rm -rf {} +
ok "Extension at $EXT_OUT"

# Ad-hoc sign so Gatekeeper sees an identity. Not notarised.
step "Ad-hoc codesign"
codesign --force --deep --sign - "$APP_BUNDLE"
ok "Ad-hoc signed"

# DMG: .app + Applications symlink + extension/ + README.txt at top level.
step "Packaging DMG"
RELEASE_DIR="$SCRIPT_DIR/release"
mkdir -p "$RELEASE_DIR"
DMG_PATH="$RELEASE_DIR/HackPass-1.0.0.dmg"
rm -f "$DMG_PATH"

DMG_STAGE="$(mktemp -d)/HackPass"
mkdir -p "$DMG_STAGE"
cp -R "$APP_BUNDLE" "$DMG_STAGE/"
ln -s /Applications "$DMG_STAGE/Applications"
cp -R "$REPO_ROOT/shared/extension" "$DMG_STAGE/extension"
find "$DMG_STAGE/extension" -name node_modules -type d -prune -exec rm -rf {} +

cat > "$DMG_STAGE/README.txt" <<'TXT'
HackPass - install instructions

  1. Drag HackPass.app to the Applications folder (in this window).

  2. Open HackPass from Applications. macOS will warn that the app is
     from an unidentified developer - this is expected, the app is
     unsigned by design. Right-click HackPass.app -> Open -> Open
     to acknowledge the warning and run it.

  3. To enable browser autofill, load the included extension/ folder
     in Chrome or Edge:
       Chrome: chrome://extensions -> Developer mode -> Load unpacked
       Edge:   edge://extensions   -> Developer mode -> Load unpacked
     Point it at the extension/ folder in THIS DMG (or copy the folder
     to your Desktop first if you prefer).

  4. Open HackPass's Settings, copy the extension token, paste it
     into the extension's Options page.

WARNING: HackPass is INTENTIONALLY VULNERABLE software. It is a
research target, not a real password manager. Do not store real
credentials in it.

See https://github.com/samanL33T/HackPass for full docs.
TXT

# Prefer create-dmg (brew) for a styled DMG, fall back to hdiutil.
if command -v create-dmg >/dev/null 2>&1; then
    create-dmg \
        --volname "HackPass" \
        --window-pos 200 120 --window-size 640 400 \
        --icon-size 96 \
        --icon "HackPass.app"   140 180 \
        --icon "Applications"   500 180 \
        --icon "extension"      140 310 \
        --icon "README.txt"     500 310 \
        --hide-extension "HackPass.app" \
        --no-internet-enable \
        "$DMG_PATH" "$DMG_STAGE" || warn "create-dmg failed; falling back to hdiutil"
fi
if [[ ! -f "$DMG_PATH" ]]; then
    hdiutil create -volname "HackPass" -srcfolder "$DMG_STAGE" -ov \
        -format UDZO -fs HFS+ "$DMG_PATH"
fi
rm -rf "$(dirname "$DMG_STAGE")"
ok "DMG at $DMG_PATH ($(du -sh "$DMG_PATH" | cut -f1))"

echo
echo "==> Release artifact"
echo "    $DMG_PATH"
echo
echo "Unsigned/un-notarised. Right-click -> Open on first launch to acknowledge the warning."
echo "If a previously pinned HackPass shows a stale icon in the Dock: killall Dock"
