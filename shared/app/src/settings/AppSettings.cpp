#include "AppSettings.h"

#include "crypto/LegacyXOR.h"

namespace {
constexpr auto kTheme            = "theme";
constexpr auto kAutoLockMinutes  = "auto_lock_minutes";
constexpr auto kRecentVaultPath  = "recent_vault_path";
constexpr auto kWindowGeometry   = "window_geometry";
constexpr auto kHardeningEnabled = "hardening_enabled";
constexpr auto kServerUrl        = "server_url";
constexpr auto kSyncToken        = "sync_token";
constexpr auto kLastSyncAt       = "last_sync_at";
constexpr auto kDeviceId         = "device_id";
}

AppSettings::AppSettings(QObject* parent)
    : QObject(parent),
      m_settings(QSettings::NativeFormat, QSettings::UserScope,
                 QStringLiteral("HackPass"), QStringLiteral("app")) {}

QString AppSettings::theme() const {
    return m_settings.value(kTheme, QStringLiteral("dark")).toString();
}

void AppSettings::setTheme(const QString& v) {
    if (v == theme()) return;
    m_settings.setValue(kTheme, v);
    emit themeChanged();
}

int AppSettings::autoLockMinutes() const {
    return m_settings.value(kAutoLockMinutes, 5).toInt();
}

void AppSettings::setAutoLockMinutes(int v) {
    if (v == autoLockMinutes()) return;
    m_settings.setValue(kAutoLockMinutes, v);
    emit autoLockMinutesChanged();
}

QString AppSettings::recentVaultPath() const {
    return m_settings.value(kRecentVaultPath).toString();
}

void AppSettings::setRecentVaultPath(const QString& v) {
    if (v == recentVaultPath()) return;
    m_settings.setValue(kRecentVaultPath, v);
    emit recentVaultPathChanged();
}

QByteArray AppSettings::windowGeometry() const {
    return m_settings.value(kWindowGeometry).toByteArray();
}

void AppSettings::setWindowGeometry(const QByteArray& v) {
    m_settings.setValue(kWindowGeometry, v);
}

bool AppSettings::hardeningEnabled() const {
    return m_settings.value(kHardeningEnabled, false).toBool();
}

void AppSettings::setHardeningEnabled(bool v) {
    if (v == hardeningEnabled()) return;
    m_settings.setValue(kHardeningEnabled, v);
    emit hardeningEnabledChanged();
}

QString AppSettings::serverUrl() const {
    return m_settings.value(kServerUrl, QStringLiteral("https://localhost:8443")).toString();
}

void AppSettings::setServerUrl(const QString& v) {
    if (v == serverUrl()) return;
    m_settings.setValue(kServerUrl, v);
    emit serverUrlChanged();
}

QString AppSettings::syncToken() const {
    const QByteArray b64 = m_settings.value(kSyncToken).toByteArray();
    if (b64.isEmpty()) return {};
    const QByteArray cipher = QByteArray::fromBase64(b64);
    const QByteArray plain  = LegacyXOR::decrypt(cipher);
    return QString::fromUtf8(plain);
}

void AppSettings::setSyncToken(const QString& v) {
    if (v.isEmpty()) {
        m_settings.remove(kSyncToken);
        return;
    }
    const QByteArray cipher = LegacyXOR::encrypt(v.toUtf8());
    m_settings.setValue(kSyncToken, cipher.toBase64());
}

qint64 AppSettings::lastSyncAt() const {
    return m_settings.value(kLastSyncAt, 0).toLongLong();
}

void AppSettings::setLastSyncAt(qint64 ms) {
    m_settings.setValue(kLastSyncAt, ms);
}

QString AppSettings::deviceId() const {
    return m_settings.value(kDeviceId).toString();
}

void AppSettings::setDeviceId(const QString& v) {
    if (v == deviceId()) return;
    m_settings.setValue(kDeviceId, v);
    emit deviceIdChanged();
}

void AppSettings::sync()  { m_settings.sync(); }
void AppSettings::clear() { m_settings.clear(); }
