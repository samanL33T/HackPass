#include "PlatformUiMacOS.h"

#include <QWindow>

#import <AppKit/AppKit.h>

void PlatformUiMacOS::applyAppStyleHints() {
    if (@available(macOS 10.14, *)) {
        [NSApp setAppearance:[NSAppearance appearanceNamed:NSAppearanceNameDarkAqua]];
    }
}

void PlatformUiMacOS::applyWindowChromeHints(QWindow* window) {
    Q_UNUSED(window);
}

void PlatformUiMacOS::registerGlobalHotkeys(QWindow* window) {
    Q_UNUSED(window);
    // Not implemented.
}

QString PlatformUiMacOS::defaultBrowserExecutable() const {
    return QStringLiteral("/Applications/Google Chrome.app/Contents/MacOS/Google Chrome");
}
