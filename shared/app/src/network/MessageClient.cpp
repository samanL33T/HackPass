#include "MessageClient.h"

#include "PinnedNetworkAccessManager.h"
#include "settings/AppSettings.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

MessageClient::MessageClient(PinnedNetworkAccessManager* nam, AppSettings* settings, QObject* parent)
    : QObject(parent), m_nam(nam), m_settings(settings) {}

void MessageClient::fetch() {
    if (!m_nam) {
        emit messagesFailed("network manager unavailable");
        return;
    }
    const QString base = m_settings ? m_settings->serverUrl()
                                    : QStringLiteral("https://localhost:8443");
    QNetworkRequest req(QUrl(base + QStringLiteral("/api/v1/messages")));
    QNetworkReply* reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit messagesFailed(reply->errorString());
            return;
        }
        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isObject()) {
            emit messagesFailed("invalid response");
            return;
        }
        QStringList msgs;
        for (const auto& v : doc.object().value("messages").toArray()) {
            msgs.append(v.toString());
        }
        emit messagesReceived(msgs);
    });
}
