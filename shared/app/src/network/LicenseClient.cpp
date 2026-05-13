#include "LicenseClient.h"

#include "PinnedNetworkAccessManager.h"
#include "settings/AppSettings.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

LicenseClient::LicenseClient(PinnedNetworkAccessManager* nam, AppSettings* settings, QObject* parent)
    : QObject(parent), m_nam(nam), m_settings(settings) {}

void LicenseClient::validate(const QString& licenseKey) {
    if (!m_nam) {
        emit validateFailed("network manager unavailable");
        return;
    }
    const QString base = m_settings ? m_settings->serverUrl()
                                    : QStringLiteral("https://localhost:8443");
    QNetworkRequest req(QUrl(base + QStringLiteral("/api/v1/license/validate")));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject body;
    body["license_key"] = licenseKey;
    QNetworkReply* reply = m_nam->post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit validateFailed(reply->errorString());
            return;
        }
        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isObject()) {
            emit validateFailed(QStringLiteral("invalid response"));
            return;
        }
        const auto o = doc.object();
        const QString tier = o.value("tier").toString();
        const qint64 expiresMs = static_cast<qint64>(o.value("expires_ms").toDouble());
        emit validateSucceeded(tier, expiresMs);
    });
}
