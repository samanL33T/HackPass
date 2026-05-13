#!/usr/bin/env bash
# Prereqs -> cmake -> macdeployqt -> dev backend. Flags: --skip-prereq, --skip-app, --skip-backend, --clean.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

SKIP_PREREQ=false
SKIP_APP=false
SKIP_BACKEND=false
CLEAN=false

for arg in "$@"; do
    case "$arg" in
        --skip-prereq)   SKIP_PREREQ=true ;;
        --skip-app)      SKIP_APP=true ;;
        --skip-backend)  SKIP_BACKEND=true ;;
        --clean)         CLEAN=true ;;
        *) echo "unknown arg: $arg" >&2; exit 1 ;;
    esac
done

step()  { printf "\n==> %s\n" "$1"; }
ok()    { printf "    ok: %s\n" "$1"; }
warn()  { printf "    warn: %s\n" "$1"; }

if ! $SKIP_PREREQ; then
    step "Checking prerequisites"

    if command -v cmake >/dev/null; then
        ok "cmake: $(cmake --version | head -n1)"
    else
        echo "    error: cmake missing. brew install cmake"; exit 1
    fi

    if xcode-select -p >/dev/null 2>&1; then
        ok "Xcode Command Line Tools at $(xcode-select -p)"
    else
        echo "    error: Xcode CLT missing. xcode-select --install"; exit 1
    fi

    QT_DIR="${Qt6_DIR:-}"
    if [[ -z "$QT_DIR" ]]; then
        shopt -s nullglob
        for candidate in \
            "$HOME"/Qt/*/macos \
            "$HOME"/*/Qt/*/macos \
            /opt/Qt/*/macos \
            /Applications/Qt/*/macos
        do
            if [[ -x "$candidate/bin/qmake" ]]; then
                QT_DIR="$candidate"
                break
            fi
        done
        shopt -u nullglob
    fi
    if [[ -n "$QT_DIR" ]]; then
        ok "Qt at $QT_DIR"
        export CMAKE_PREFIX_PATH="$QT_DIR"
        export PATH="$QT_DIR/bin:$PATH"
        if [[ ! -f "$QT_DIR/lib/cmake/Qt6WebSockets/Qt6WebSocketsConfig.cmake" ]]; then
            echo "    error: Qt WebSockets module not found at $QT_DIR/lib/cmake/Qt6WebSockets/."
            echo "           Open ~/Qt/MaintenanceTool.app and tick: Qt $(basename "$QT_DIR" | sed 's|.*Qt/||;s|/macos||') -> Additional Libraries -> Qt WebSockets"
            exit 1
        fi
        ok "Qt WebSockets module present"
    else
        echo "    error: Qt 6.7+ for macOS not found. Set Qt6_DIR or install via the Qt online installer."; exit 1
    fi

    if [[ -d "/opt/homebrew/opt/openssl@3" ]]; then
        export OPENSSL_ROOT_DIR="/opt/homebrew/opt/openssl@3"
        ok "OpenSSL at $OPENSSL_ROOT_DIR"
    elif [[ -d "/usr/local/opt/openssl@3" ]]; then
        export OPENSSL_ROOT_DIR="/usr/local/opt/openssl@3"
        ok "OpenSSL at $OPENSSL_ROOT_DIR"
    else
        warn "OpenSSL not at homebrew default. CMake will try its bundled finder."
    fi

    if command -v bun >/dev/null; then
        ok "bun: $(bun --version)"
    else
        warn "bun not on PATH. Dev backend cannot be started; install with: curl -fsSL https://bun.sh/install | bash"
    fi
fi

if ! $SKIP_APP; then
    cd "$REPO_ROOT"

    if $CLEAN && [[ -d build ]]; then
        step "Cleaning build/"
        rm -rf build
    fi

    # Generate the bundle icon if missing, so the Dock shows the right icon.
    if [[ ! -f "$REPO_ROOT/mac/installer/HackPass.icns" ]]; then
        step "Generating HackPass.icns"
        bash "$REPO_ROOT/shared/tools/generate-icns.sh"
    fi

    step "Configuring (cmake)"
    cmake -S . -B build -G "Unix Makefiles" \
        -DCMAKE_BUILD_TYPE=Release \
        ${CMAKE_PREFIX_PATH:+-DCMAKE_PREFIX_PATH="$CMAKE_PREFIX_PATH"} \
        ${OPENSSL_ROOT_DIR:+-DOPENSSL_ROOT_DIR="$OPENSSL_ROOT_DIR"}

    step "Building (cmake --build)"
    cmake --build build -j

    APP_BUNDLE="$REPO_ROOT/build/shared/app/HackPass.app"
    if [[ ! -d "$APP_BUNDLE" ]]; then
        APP_BUNDLE="$(find "$REPO_ROOT/build" -name HackPass.app -type d | head -n1)"
    fi
    if [[ -z "$APP_BUNDLE" || ! -d "$APP_BUNDLE" ]]; then
        echo "    error: HackPass.app not found under build/"; exit 1
    fi
    ok "Built bundle: $APP_BUNDLE"

    step "Bundling Qt frameworks (macdeployqt)"
    macdeployqt "$APP_BUNDLE" -qmldir="$REPO_ROOT/shared/app/src/resources/qml" -verbose=1
    ok "macdeployqt complete"

    # Drop unused Qt cruft from the .app (mirrors the Windows slim pass).
    step "Slimming Qt bundle"
    QML_DIR="$APP_BUNDLE/Contents/Resources/qml/QtQuick"
    if [[ -d "$QML_DIR" ]]; then
        for style in Imagine Fusion Universal FluentWinUI3 Windows; do
            if [[ -d "$QML_DIR/Controls/$style" ]]; then
                rm -rf "$QML_DIR/Controls/$style"
                ok "  dropped qml/QtQuick/Controls/$style/"
            fi
        done
        for sub in NativeStyle Particles Dialogs Timeline LocalStorage VectorImage; do
            if [[ -d "$QML_DIR/$sub" ]]; then
                rm -rf "$QML_DIR/$sub"
                ok "  dropped qml/QtQuick/$sub/"
            fi
        done
    fi
    if [[ -d "$APP_BUNDLE/Contents/Resources/qml/QtQml/XmlListModel" ]]; then
        rm -rf "$APP_BUNDLE/Contents/Resources/qml/QtQml/XmlListModel"
        ok "  dropped qml/QtQml/XmlListModel/"
    fi
    if [[ -d "$APP_BUNDLE/Contents/PlugIns/qmltooling" ]]; then
        rm -rf "$APP_BUNDLE/Contents/PlugIns/qmltooling"
        ok "  dropped PlugIns/qmltooling/"
    fi
    IMG_FMT="$APP_BUNDLE/Contents/PlugIns/imageformats"
    if [[ -d "$IMG_FMT" ]]; then
        for f in libqgif.dylib libqicns.dylib libqico.dylib libqjpeg.dylib \
                 libqtga.dylib libqtiff.dylib libqwbmp.dylib libqwebp.dylib \
                 libqpdf.dylib libqmacheif.dylib libqmacjp2.dylib; do
            if [[ -f "$IMG_FMT/$f" ]]; then
                rm -f "$IMG_FMT/$f"
                ok "  dropped imageformats/$f"
            fi
        done
    fi
    TRANS="$APP_BUNDLE/Contents/Resources/translations"
    if [[ -d "$TRANS" ]]; then
        find "$TRANS" -name "qt_*.qm" -not -name "qt_en.qm" -delete 2>/dev/null
        ok "  trimmed translations to qt_en.qm only"
    fi
fi

if ! $SKIP_BACKEND; then
    if command -v bun >/dev/null; then
        step "Starting backend with bun (background)"
        (cd "$REPO_ROOT/shared/backend" && bun install --silent && bun run start >/tmp/hackpass-backend.log 2>&1 &)
        ok "Backend launched; logs at /tmp/hackpass-backend.log"

        defaults write com.samanl33t.hackpass server_url -string "https://localhost:8443"
        ok "Dev mode: server_url set to https://localhost:8443 (~/Library/Preferences/com.samanl33t.hackpass.plist)"
    else
        warn "bun missing; backend not started. Install bun and pass --skip-app to rerun just this stage."
    fi
fi

echo
step "Done"
if ! $SKIP_APP; then
    echo "  App:       $APP_BUNDLE"
fi
if ! $SKIP_BACKEND; then
    echo "  Backend:   https://localhost:8443"
fi
echo "  Extension: chrome://extensions -> Developer mode -> Load unpacked -> select $REPO_ROOT/shared/extension"
echo
echo "Reminder: HackPass is intentionally vulnerable. Do not store real passwords."
