#include "PlatformPathsMacOS.h"

#include <QDir>
#include <QStandardPaths>

QString PlatformPathsMacOS::settingsScopeKey() const {
    return QStringLiteral("HackPass");
}

QString PlatformPathsMacOS::defaultVaultDir() const {
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return QDir(base).filePath(QStringLiteral("vaults"));
}

QString PlatformPathsMacOS::configDir() const {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
}

QString PlatformPathsMacOS::cacheDir() const {
    return QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
}

QString PlatformPathsMacOS::ipcEndpointDescriptor() const {
    return QStringLiteral("ws://127.0.0.1:8765");
}

QString PlatformPathsMacOS::tofuStorePath() const {
    return QDir(configDir()).filePath(QStringLiteral("tofu.json"));
}
