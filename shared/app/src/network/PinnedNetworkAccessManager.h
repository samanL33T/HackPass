#pragma once

#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QSslError>

class TofuStore;

// QNetworkAccessManager subclass that pins TLS certs via TOFU.
//
// First connection to a host: capture SHA-256(peer cert pubkey) and store it.
// Subsequent connections: reject any handshake whose peer cert SHA-256 does
// not match. Three hookable points: the sslErrors slot, TofuStore::matches,
// and the cert-hash compute.
class PinnedNetworkAccessManager : public QNetworkAccessManager {
    Q_OBJECT
public:
    PinnedNetworkAccessManager(TofuStore* tofu, QObject* parent = nullptr);

    // If true, sslErrors are silently ignored on first connect so the server's
    // cert fingerprint gets pinned. Should only be on briefly, around a single
    // outbound request. QML toggles this from Settings -> Test Connection.
    Q_INVOKABLE void setAllowTofuLearning(bool allow) { m_allowTofuLearning = allow; }
    Q_INVOKABLE bool allowTofuLearning() const         { return m_allowTofuLearning; }

private slots:
    void onSslErrors(QNetworkReply* reply, const QList<QSslError>& errors);
    void onEncrypted(QNetworkReply* reply);

private:
    static QByteArray sha256OfCertPubKey(const QSslCertificate& cert);

    TofuStore* m_tofu               = nullptr;
    bool       m_allowTofuLearning  = false;
};
