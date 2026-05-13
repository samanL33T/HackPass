#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

class PinnedNetworkAccessManager;
class AppSettings;

class MessageClient : public QObject {
    Q_OBJECT
public:
    MessageClient(PinnedNetworkAccessManager* nam, AppSettings* settings, QObject* parent = nullptr);

    void fetch();

signals:
    void messagesReceived(QStringList messages);
    void messagesFailed(QString reason);

private:
    PinnedNetworkAccessManager* m_nam      = nullptr;
    AppSettings*                m_settings = nullptr;
};
