#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>

class AppSettings;
class LicenseGate;
class PremiumGate;
class ServerFlagsApplier;
class VaultManager;

// The QML <-> C++ bridge surface. Fourteen Q_INVOKABLE methods total: four real
// attack vectors hidden among ten realistic-looking noise / decoy methods.
// The real ones are: setPremium, applyServerFlags, restoreFromBackup,
// setHardeningEnabled. The noise mimics debug remnants real password manager
// developers tend to leave in production builds.
class SettingsBridge : public QObject {
    Q_OBJECT
    Q_PROPERTY(int    autoLockMinutes READ autoLockMinutes WRITE setAutoLockMinutes NOTIFY autoLockMinutesChanged)
    Q_PROPERTY(QString currentTheme   READ currentTheme    WRITE setCurrentTheme    NOTIFY currentThemeChanged)

public:
    SettingsBridge(AppSettings*        settings,
                   LicenseGate*        licenseGate,
                   PremiumGate*        premiumGate,
                   ServerFlagsApplier* serverFlagsApplier,
                   VaultManager*       vaultManager,
                   QObject*            parent = nullptr);

    int     autoLockMinutes() const;
    void    setAutoLockMinutes(int v);

    QString currentTheme() const;
    void    setCurrentTheme(const QString& v);

    // === Real attack vectors ===
    Q_INVOKABLE void    setPremium(bool enabled);                 // direct flag flip
    Q_INVOKABLE bool    isPremium() const;
    Q_INVOKABLE void    applyServerFlags(const QVariantMap& fl); // reachable from QML
    Q_INVOKABLE bool    restoreFromBackup(const QString& path);   // no path validation
    Q_INVOKABLE void    setHardeningEnabled(bool enabled);

    // === Plausible-but-empty noise ===
    Q_INVOKABLE QString debugDumpState();
    Q_INVOKABLE void    internalLog(const QString& message);
    Q_INVOKABLE bool    legacyValidate(const QString& a, const QString& b);
    Q_INVOKABLE QString getInternalVersion();
    Q_INVOKABLE bool    metricsEnabled();
    Q_INVOKABLE void    pulseHeartbeat();
    Q_INVOKABLE QStringList listFeatureFlags();
    Q_INVOKABLE void    requestFactoryReset();
    Q_INVOKABLE QString validateLicense(const QString& key);

signals:
    void autoLockMinutesChanged();
    void currentThemeChanged();

private:
    AppSettings*        m_settings           = nullptr;
    LicenseGate*        m_licenseGate        = nullptr;
    PremiumGate*        m_premiumGate        = nullptr;
    ServerFlagsApplier* m_serverFlagsApplier = nullptr;
    VaultManager*       m_vaultManager       = nullptr;
};
