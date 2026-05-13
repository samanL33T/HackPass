#include "SyncClient.h"

#include "PinnedNetworkAccessManager.h"
#include "settings/AppSettings.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

SyncClient::SyncClient(PinnedNetworkAccessManager* nam, AppSettings* settings, QObject* parent)
    : QObject(parent), m_nam(nam), m_settings(settings) {}

QString SyncClient::buildUrl(const QString& path) const {
    const QString base = m_settings ? m_settings->serverUrl()
                                    : QStringLiteral("https://localhost:8443");
    return base + path;
}

void SyncClient::pull(const QString& vaultId) {
    if (!m_nam) {
        emit pullFailed("network manager unavailable");
        return;
    }
    QNetworkRequest req(QUrl(buildUrl(QStringLiteral("/api/v1/vaults/") + vaultId)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply* reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit pullFailed(reply->errorString());
            return;
        }
        const QByteArray body = reply->readAll();
        const QJsonDocument doc = QJsonDocument::fromJson(body);
        if (!doc.isObject()) {
            emit pullFailed(QStringLiteral("invalid response"));
            return;
        }
        const QJsonObject o = doc.object();
        const QByteArray blob = QByteArray::fromBase64(o.value("vault").toString().toLatin1());
        const qint64 version  = static_cast<qint64>(o.value("version").toDouble());
        QVariantMap flags;
        if (o.contains("server_flags") && o.value("server_flags").isObject()) {
            flags = o.value("server_flags").toObject().toVariantMap();
        }
        emit serverFlagsReceived(flags);
        emit pullSucceeded(blob, version, flags);
    });
}

void SyncClient::push(const QString& vaultId, const QByteArray& encryptedVaultBytes, qint64 version) {
    if (!m_nam) {
        emit pushFailed("network manager unavailable");
        return;
    }
    QJsonObject body;
    body["vault"]   = QString::fromLatin1(encryptedVaultBytes.toBase64());
    body["version"] = static_cast<double>(version);
    const QByteArray payload = QJsonDocument(body).toJson(QJsonDocument::Compact);

    QNetworkRequest req(QUrl(buildUrl(QStringLiteral("/api/v1/vaults/") + vaultId)));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply* reply = m_nam->put(req, payload);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit pushFailed(reply->errorString());
            return;
        }
        const QByteArray respBody = reply->readAll();
        const QJsonDocument doc = QJsonDocument::fromJson(respBody);
        const qint64 newVersion = doc.isObject()
            ? static_cast<qint64>(doc.object().value("version").toDouble())
            : 0;
        emit pushSucceeded(newVersion);
    });
}
