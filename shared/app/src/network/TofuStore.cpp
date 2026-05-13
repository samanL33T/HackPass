#include "TofuStore.h"

#include <openssl/crypto.h>

TofuStore::TofuStore(QObject* parent)
    : QObject(parent),
      m_store(QSettings::NativeFormat, QSettings::UserScope,
              QStringLiteral("HackPass"), QStringLiteral("tofu")) {}

QByteArray TofuStore::fingerprint(const QString& host) const {
    return m_store.value(host).toByteArray();
}

void TofuStore::setFingerprint(const QString& host, const QByteArray& sha256) {
    m_store.setValue(host, sha256);
}

void TofuStore::forget(const QString& host) {
    m_store.remove(host);
}

bool TofuStore::matches(const QString& host, const QByteArray& candidate) const {
    const QByteArray stored = fingerprint(host);
    if (stored.isEmpty() || stored.size() != candidate.size()) {
        return false;
    }
    return CRYPTO_memcmp(stored.constData(), candidate.constData(),
                         static_cast<size_t>(stored.size())) == 0;
}
