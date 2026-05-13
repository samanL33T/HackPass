#pragma once

#include <QByteArray>
#include <QObject>
#include <QSettings>
#include <QString>

// Stores the browser-extension handshake token. Intentionally writes the token
// plaintext into the registry so any local process under the same user can read
// it. This is the lesson surface for the IPC post.
class TokenStore : public QObject {
    Q_OBJECT
public:
    explicit TokenStore(QObject* parent = nullptr);

    Q_INVOKABLE QString token() const;
    Q_INVOKABLE void    setToken(const QString& token);
    Q_INVOKABLE void    clear();

    qint64 installedAt() const;
    void   stampInstalledNow();

    // Generates 32 hex chars from RAND_bytes and stores them. Returns the new token.
    Q_INVOKABLE QString rotate();

private:
    QSettings m_settings;
};
