#include "PinnedNetworkAccessManager.h"

#include "TofuStore.h"

#include <QCryptographicHash>
#include <QSslCertificate>
#include <QSslConfiguration>
#include <QSslKey>
#include <QUrl>

PinnedNetworkAccessManager::PinnedNetworkAccessManager(TofuStore* tofu, QObject* parent)
    : QNetworkAccessManager(parent), m_tofu(tofu) {
    connect(this, &QNetworkAccessManager::sslErrors,
            this, &PinnedNetworkAccessManager::onSslErrors);
    connect(this, &QNetworkAccessManager::encrypted,
            this, &PinnedNetworkAccessManager::onEncrypted);
}

void PinnedNetworkAccessManager::onSslErrors(QNetworkReply* reply, const QList<QSslError>& errors) {
    Q_UNUSED(errors);
    const QString host = reply->url().host();
    const QSslConfiguration cfg = reply->sslConfiguration();
    const QSslCertificate peer  = cfg.peerCertificate();
    if (peer.isNull()) {
        return;  // No peer cert; let Qt's default reject path proceed.
    }
    const QByteArray candidate = sha256OfCertPubKey(peer);

    if (m_tofu && m_tofu->matches(host, candidate)) {
        // Pinned cert matches. Ignore the errors (typically self-signed CA chain).
        reply->ignoreSslErrors();
        return;
    }
    // No matching pin. Two paths that accept-and-learn the cert here:
    //   1) Caller explicitly set allowTofuLearning (Settings -> Test Connection)
    //   2) The host has no pin yet at all (first connect to a new host)
    // Subsequent connects MUST match the pinned hash; mismatches are rejected.
    const bool hostUnknown = m_tofu && m_tofu->fingerprint(host).isEmpty();
    if ((m_allowTofuLearning || hostUnknown) && m_tofu) {
        m_tofu->setFingerprint(host, candidate);
        reply->ignoreSslErrors();
    }
}

void PinnedNetworkAccessManager::onEncrypted(QNetworkReply* reply) {
    // Defensive: even if Qt accepted the handshake (e.g. against a real public CA),
    // for our pinned hosts we want to reject if the cert hash is not the one we
    // pinned. Cancels the reply early in that case.
    const QString host = reply->url().host();
    if (!m_tofu) return;
    const QSslConfiguration cfg = reply->sslConfiguration();
    const QSslCertificate peer  = cfg.peerCertificate();
    if (peer.isNull()) return;
    const QByteArray candidate = sha256OfCertPubKey(peer);
    if (m_tofu->fingerprint(host).isEmpty()) {
        // Unknown host. If TOFU learning is on, record. Otherwise abort.
        if (m_allowTofuLearning) {
            m_tofu->setFingerprint(host, candidate);
        } else {
            reply->abort();
        }
        return;
    }
    if (!m_tofu->matches(host, candidate)) {
        reply->abort();
    }
}

QByteArray PinnedNetworkAccessManager::sha256OfCertPubKey(const QSslCertificate& cert) {
    const QByteArray der = cert.publicKey().toDer();
    return QCryptographicHash::hash(der, QCryptographicHash::Sha256);
}
