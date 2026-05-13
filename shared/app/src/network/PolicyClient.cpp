#include "PolicyClient.h"

#include "PinnedNetworkAccessManager.h"
#include "settings/AppSettings.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

PolicyClient::PolicyClient(PinnedNetworkAccessManager* nam, AppSettings* settings, QObject* parent)
    : QObject(parent), m_nam(nam), m_settings(settings) {}

void PolicyClient::fetch() {
    if (!m_nam) {
        emit policyFailed("network manager unavailable");
        return;
    }
    const QString base = m_settings ? m_settings->serverUrl()
                                    : QStringLiteral("https://localhost:8443");
    QNetworkRequest req(QUrl(base + QStringLiteral("/api/v1/policy")));
    QNetworkReply* reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit policyFailed(reply->errorString());
            return;
        }
        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isObject()) {
            emit policyFailed("invalid response");
            return;
        }
        QVariantMap flags;
        if (doc.object().contains("server_flags")) {
            flags = doc.object().value("server_flags").toObject().toVariantMap();
        }
        emit policyReceived(flags);
    });
}
