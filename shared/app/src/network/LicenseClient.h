#pragma once

#include <QObject>
#include <QString>

class PinnedNetworkAccessManager;
class AppSettings;

class LicenseClient : public QObject {
    Q_OBJECT
public:
    LicenseClient(PinnedNetworkAccessManager* nam, AppSettings* settings, QObject* parent = nullptr);

    void validate(const QString& licenseKey);

signals:
    void validateSucceeded(QString tier, qint64 expiresMs);
    void validateFailed(QString reason);

private:
    PinnedNetworkAccessManager* m_nam      = nullptr;
    AppSettings*                m_settings = nullptr;
};
