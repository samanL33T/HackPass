#include "Totp.h"

#include <QDateTime>

#include <openssl/evp.h>
#include <openssl/hmac.h>

namespace {

// Decodes base32 (RFC 4648 alphabet) to bytes. Tolerates lowercase,
// padding, and whitespace; skips any unrecognised character silently.
QByteArray decodeBase32(const QString& s) {
    static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    QByteArray out;
    int buf  = 0;
    int bits = 0;
    for (QChar qc : s) {
        const char c = qc.toUpper().toLatin1();
        if (c == '=' || c == ' ' || c == '\t' || c == '\r' || c == '\n') continue;
        const char* p = std::strchr(alphabet, c);
        if (!p) continue;
        const int v = static_cast<int>(p - alphabet);
        buf  = (buf << 5) | v;
        bits += 5;
        if (bits >= 8) {
            out.append(static_cast<char>((buf >> (bits - 8)) & 0xFF));
            bits -= 8;
        }
    }
    return out;
}

}  // namespace

Totp::Totp(QObject* parent) : QObject(parent) {}

QString Totp::generateCode(const QString& base32Secret, qint64 epochMs) const {
    if (epochMs == 0) {
        epochMs = QDateTime::currentMSecsSinceEpoch();
    }
    qint64 counter = (epochMs / 1000) / 30;

    const QByteArray key = decodeBase32(base32Secret);
    if (key.isEmpty()) {
        return QStringLiteral("------");
    }

    // Pack counter as 8-byte big-endian.
    unsigned char ctr[8];
    for (int i = 7; i >= 0; --i) {
        ctr[i] = static_cast<unsigned char>(counter & 0xFF);
        counter >>= 8;
    }

    unsigned int outLen = 0;
    unsigned char tag[EVP_MAX_MD_SIZE];
    if (!HMAC(EVP_sha1(),
              key.constData(), key.size(),
              ctr, sizeof(ctr),
              tag, &outLen) || outLen < 20) {
        return QStringLiteral("------");
    }

    // RFC 4226 dynamic truncation.
    const int offset  = tag[outLen - 1] & 0x0F;
    const int code    = ((tag[offset]     & 0x7F) << 24)
                      | ((tag[offset + 1] & 0xFF) << 16)
                      | ((tag[offset + 2] & 0xFF) << 8)
                      |  (tag[offset + 3] & 0xFF);

    return QString::asprintf("%06d", code % 1000000);
}

int Totp::secondsRemaining(qint64 epochMs) const {
    if (epochMs == 0) {
        epochMs = QDateTime::currentMSecsSinceEpoch();
    }
    const int secInWindow = static_cast<int>((epochMs / 1000) % 30);
    return 30 - secInWindow;
}
