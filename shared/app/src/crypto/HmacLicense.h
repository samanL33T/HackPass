#pragma once

#include <QByteArray>
#include <QDateTime>
#include <QString>
#include <optional>

struct LicensePayload {
    QString    user;       // licensee identifier
    QDateTime  expires;    // UTC expiry; QDateTime::isValid must be true
    QString    tier;       // "free" | "pro"
};

class HmacLicense {
public:
    // Returns blob = base64(json_payload) + "." + base64(hmac_sha256(json_payload, key)).
    // Returns empty QByteArray on payload validation failure.
    static QByteArray sign(const LicensePayload& payload, const QByteArray& key);

    // Returns the parsed payload on success, std::nullopt on signature or schema failure.
    // The tag comparison is constant-time (CRYPTO_memcmp).
    static std::optional<LicensePayload> verify(const QByteArray& blob, const QByteArray& key);

    // Returns true if `payload.expires` is in the past relative to `now`.
    // Helper kept separate so the caller can decide whether expiry should block.
    static bool isExpired(const LicensePayload& payload, const QDateTime& now = QDateTime::currentDateTimeUtc());
};
