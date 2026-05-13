#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>

class PremiumGate;
class AppSettings;

// Applies server_flags + policy_message from the backend response. Reachable
// from QML (lesson) AND from SyncClient/PolicyClient (legitimate path).
class ServerFlagsApplier : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool exportPlaintextAllowed READ exportPlaintextAllowed NOTIFY exportPlaintextAllowedChanged)
    Q_PROPERTY(bool legacyKdfAllowed       READ legacyKdfAllowed       NOTIFY legacyKdfAllowedChanged)
    Q_PROPERTY(bool forceRelockRequired    READ forceRelockRequired    NOTIFY forceRelockRequiredChanged)
    Q_PROPERTY(QString policyMessage       READ policyMessage          NOTIFY policyMessageChanged)
    Q_PROPERTY(QString deviceStatus        READ deviceStatus           NOTIFY deviceStatusChanged)

public:
    ServerFlagsApplier(PremiumGate* premiumGate, AppSettings* settings, QObject* parent = nullptr);

    bool    exportPlaintextAllowed() const { return m_exportPlaintextAllowed; }
    bool    legacyKdfAllowed()       const { return m_legacyKdfAllowed; }
    bool    forceRelockRequired()    const { return m_forceRelockRequired; }
    QString policyMessage()          const { return m_policyMessage; }
    QString deviceStatus()           const { return m_deviceStatus; }

    Q_INVOKABLE void apply(const QVariantMap& flags);
    Q_INVOKABLE void reset();

signals:
    void exportPlaintextAllowedChanged();
    void legacyKdfAllowedChanged();
    void forceRelockRequiredChanged();
    void policyMessageChanged();
    void deviceStatusChanged();
    void deviceRevoked();

private:
    PremiumGate*  m_premiumGate = nullptr;
    AppSettings*  m_settings    = nullptr;

    bool    m_exportPlaintextAllowed = false;
    bool    m_legacyKdfAllowed       = false;
    bool    m_forceRelockRequired    = false;
    QString m_policyMessage;
    QString m_deviceStatus = QStringLiteral("trusted");
};
