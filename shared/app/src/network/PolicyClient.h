#pragma once

#include <QObject>
#include <QVariantMap>

class PinnedNetworkAccessManager;
class AppSettings;

class PolicyClient : public QObject {
    Q_OBJECT
public:
    PolicyClient(PinnedNetworkAccessManager* nam, AppSettings* settings, QObject* parent = nullptr);

    Q_INVOKABLE void fetch();

signals:
    void policyReceived(QVariantMap serverFlags);
    void policyFailed(QString reason);

private:
    PinnedNetworkAccessManager* m_nam      = nullptr;
    AppSettings*                m_settings = nullptr;
};
