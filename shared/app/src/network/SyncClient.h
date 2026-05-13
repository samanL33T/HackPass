#pragma once

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QVariantMap>

class PinnedNetworkAccessManager;
class AppSettings;

// Vault sync client. Pulls/pushes the encrypted vault blob to the backend.
// On a successful pull, emits serverFlagsReceived so ServerFlagsApplier can
// trigger downstream UI changes (the master-pw phishing surface).
class SyncClient : public QObject {
    Q_OBJECT
public:
    SyncClient(PinnedNetworkAccessManager* nam,
               AppSettings*                settings,
               QObject*                    parent = nullptr);

    // Async: fetches /api/v1/vaults/{vault_id}. Emits pullSucceeded or pullFailed.
    void pull(const QString& vaultId);

    // Async: PUTs /api/v1/vaults/{vault_id} with the encrypted blob. Emits
    // pushSucceeded or pushFailed.
    void push(const QString& vaultId, const QByteArray& encryptedVaultBytes, qint64 version);

signals:
    void pullSucceeded(QByteArray encryptedBlob, qint64 version, QVariantMap serverFlags);
    void pullFailed(QString reason);
    void pushSucceeded(qint64 newVersion);
    void pushFailed(QString reason);
    void serverFlagsReceived(QVariantMap flags);

private:
    QString buildUrl(const QString& path) const;

    PinnedNetworkAccessManager* m_nam      = nullptr;
    AppSettings*                m_settings = nullptr;
};
