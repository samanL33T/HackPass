#include "PlatformUiWindows.h"

#include <QGuiApplication>
#include <QQuickStyle>
#include <QWindow>

#include <windows.h>
#include <dwmapi.h>

void PlatformUiWindows::applyAppStyleHints() {
    QQuickStyle::setStyle(QStringLiteral("Material"));
    qputenv("QT_QUICK_CONTROLS_MATERIAL_VARIANT", "Dense");
    qputenv("QT_QUICK_CONTROLS_MATERIAL_THEME",   "Dark");
}

void PlatformUiWindows::applyWindowChromeHints(QWindow* window) {
    if (!window) return;
    const HWND hwnd = reinterpret_cast<HWND>(window->winId());
    BOOL enableDark = TRUE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &enableDark, sizeof(enableDark));
}

void PlatformUiWindows::registerGlobalHotkeys(QWindow* window) {
    Q_UNUSED(window);
    // Lock-vault and quick-find chord registration is not implemented yet.
}

QString PlatformUiWindows::defaultBrowserExecutable() const {
    return QStringLiteral("C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe");
}
