#include "PlatformPathsWindows.h"

#include <QDir>
#include <QStandardPaths>

namespace {
constexpr auto kRegistryRoot = "HKEY_CURRENT_USER\\Software\\HackPass";
constexpr auto kIpcEndpoint  = "127.0.0.1:8765";
}

QString PlatformPathsWindows::settingsScopeKey() const {
    return QStringLiteral("%1\\app").arg(kRegistryRoot);
}

QString PlatformPathsWindows::defaultVaultDir() const {
    const QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return QDir(appData).filePath(QStringLiteral("vaults"));
}

QString PlatformPathsWindows::configDir() const {
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
}

QString PlatformPathsWindows::cacheDir() const {
    return QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
}

QString PlatformPathsWindows::ipcEndpointDescriptor() const {
    return QString::fromLatin1(kIpcEndpoint);
}

QString PlatformPathsWindows::tofuStorePath() const {
    return QStringLiteral("%1\\tofu").arg(kRegistryRoot);
}
