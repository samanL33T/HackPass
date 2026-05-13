#include "SettingsBridge.h"

#include "LicenseGate.h"
#include "PremiumGate.h"
#include "ServerFlagsApplier.h"
#include "settings/AppSettings.h"
#include "vault/VaultManager.h"

#include <QCoreApplication>
#include <QFile>

SettingsBridge::SettingsBridge(AppSettings*        settings,
                               LicenseGate*        licenseGate,
                               PremiumGate*        premiumGate,
                               ServerFlagsApplier* serverFlagsApplier,
                               VaultManager*       vaultManager,
                               QObject*            parent)
    : QObject(parent),
      m_settings(settings),
      m_licenseGate(licenseGate),
      m_premiumGate(premiumGate),
      m_serverFlagsApplier(serverFlagsApplier),
      m_vaultManager(vaultManager) {
    if (m_settings) {
        connect(m_settings, &AppSettings::autoLockMinutesChanged,
                this, &SettingsBridge::autoLockMinutesChanged);
        connect(m_settings, &AppSettings::themeChanged,
                this, &SettingsBridge::currentThemeChanged);
    }
}

int SettingsBridge::autoLockMinutes() const {
    return m_settings ? m_settings->autoLockMinutes() : 5;
}

void SettingsBridge::setAutoLockMinutes(int v) {
    if (m_settings) {
        m_settings->setAutoLockMinutes(v);
    }
}

QString SettingsBridge::currentTheme() const {
    return m_settings ? m_settings->theme() : QStringLiteral("dark");
}

void SettingsBridge::setCurrentTheme(const QString& v) {
    if (m_settings) {
        m_settings->setTheme(v);
    }
}

void SettingsBridge::setPremium(bool enabled) {
    if (m_premiumGate) {
        m_premiumGate->setPremium(enabled);
    }
}

bool SettingsBridge::isPremium() const {
    return m_premiumGate ? m_premiumGate->isPremium() : false;
}

void SettingsBridge::applyServerFlags(const QVariantMap& fl) {
    if (m_serverFlagsApplier) {
        m_serverFlagsApplier->apply(fl);
    }
}

bool SettingsBridge::restoreFromBackup(const QString& path) {
    if (!m_vaultManager) return false;
    if (!QFile::exists(path)) return false;
    return m_vaultManager->selectFile(path);
}

void SettingsBridge::setHardeningEnabled(bool enabled) {
    if (m_settings) {
        m_settings->setHardeningEnabled(enabled);
    }
}

QString SettingsBridge::debugDumpState() {
    return QStringLiteral("{}");
}

void SettingsBridge::internalLog(const QString& message) {
    Q_UNUSED(message);
}

bool SettingsBridge::legacyValidate(const QString& a, const QString& b) {
    Q_UNUSED(a);
    Q_UNUSED(b);
    return false;
}

QString SettingsBridge::getInternalVersion() {
    return QCoreApplication::applicationVersion();
}

bool SettingsBridge::metricsEnabled() {
    return false;
}

void SettingsBridge::pulseHeartbeat() {}

QStringList SettingsBridge::listFeatureFlags() {
    return {
        QStringLiteral("autofill"),
        QStringLiteral("password_generator"),
        QStringLiteral("breach_check"),
        QStringLiteral("sync"),
    };
}

void SettingsBridge::requestFactoryReset() {
    // Shows a confirmation dialog in the UI; does not auto-execute.
}

QString SettingsBridge::validateLicense(const QString& key) {
    Q_UNUSED(key);
    if (m_licenseGate) {
        m_licenseGate->refresh();
        return m_licenseGate->isLicenseValid() ? QStringLiteral("valid") : QStringLiteral("invalid");
    }
    return QStringLiteral("invalid");
}
