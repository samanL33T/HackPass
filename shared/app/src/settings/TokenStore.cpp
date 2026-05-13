#include "TokenStore.h"

#include <QDateTime>

#include <openssl/rand.h>

namespace {
constexpr auto kToken       = "token";
constexpr auto kInstalledAt = "installed_at";
}

TokenStore::TokenStore(QObject* parent)
    : QObject(parent),
      m_settings(QSettings::NativeFormat, QSettings::UserScope,
                 QStringLiteral("HackPass"), QStringLiteral("extension")) {}

QString TokenStore::token() const {
    return m_settings.value(kToken).toString();
}

void TokenStore::setToken(const QString& token) {
    m_settings.setValue(kToken, token);
}

void TokenStore::clear() {
    m_settings.remove(kToken);
}

qint64 TokenStore::installedAt() const {
    return m_settings.value(kInstalledAt, 0).toLongLong();
}

void TokenStore::stampInstalledNow() {
    m_settings.setValue(kInstalledAt, QDateTime::currentMSecsSinceEpoch());
}

QString TokenStore::rotate() {
    unsigned char buf[16];
    if (RAND_bytes(buf, sizeof(buf)) != 1) {
        return {};
    }
    const QByteArray hex = QByteArray(reinterpret_cast<const char*>(buf), sizeof(buf)).toHex();
    const QString t = QString::fromLatin1(hex);
    setToken(t);
    return t;
}
