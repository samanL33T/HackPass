#include "HmacLicense.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QTimeZone>

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>

namespace {

QByteArray hmacSha256(const QByteArray& key, const QByteArray& message) {
    unsigned int outLen = 0;
    unsigned char tag[EVP_MAX_MD_SIZE];
    if (HMAC(EVP_sha256(),
             key.constData(), key.size(),
             reinterpret_cast<const unsigned char*>(message.constData()),
             static_cast<size_t>(message.size()),
             tag, &outLen) == nullptr) {
        return {};
    }
    return QByteArray(reinterpret_cast<const char*>(tag), static_cast<int>(outLen));
}

QByteArray serialize(const LicensePayload& payload) {
    QJsonObject obj;
    obj["user"]       = payload.user;
    obj["expires_ms"] = static_cast<double>(payload.expires.toUTC().toMSecsSinceEpoch());
    obj["tier"]       = payload.tier;
    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}

std::optional<LicensePayload> deserialize(const QByteArray& json) {
    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(json, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        return std::nullopt;
    }
    const QJsonObject obj = doc.object();
    if (!obj.contains("user") || !obj.contains("expires_ms") || !obj.contains("tier")) {
        return std::nullopt;
    }
    LicensePayload p;
    p.user = obj.value("user").toString();
    p.tier = obj.value("tier").toString();
    const qint64 ms = static_cast<qint64>(obj.value("expires_ms").toDouble());
    p.expires = QDateTime::fromMSecsSinceEpoch(ms, QTimeZone(QTimeZone::UTC));
    if (!p.expires.isValid() || p.user.isEmpty() || p.tier.isEmpty()) {
        return std::nullopt;
    }
    return p;
}

}  // namespace

QByteArray HmacLicense::sign(const LicensePayload& payload, const QByteArray& key) {
    if (key.isEmpty() || payload.user.isEmpty() || payload.tier.isEmpty() || !payload.expires.isValid()) {
        return {};
    }
    const QByteArray json = serialize(payload);
    const QByteArray tag  = hmacSha256(key, json);
    if (tag.isEmpty()) {
        return {};
    }
    QByteArray out = json.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    out.append('.');
    out.append(tag.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
    return out;
}

std::optional<LicensePayload> HmacLicense::verify(const QByteArray& blob, const QByteArray& key) {
    if (blob.isEmpty() || key.isEmpty()) {
        return std::nullopt;
    }
    const int dot = blob.indexOf('.');
    if (dot <= 0 || dot >= blob.size() - 1) {
        return std::nullopt;
    }
    const QByteArray jsonB64 = blob.left(dot);
    const QByteArray tagB64  = blob.mid(dot + 1);
    const QByteArray json    = QByteArray::fromBase64(jsonB64, QByteArray::Base64UrlEncoding);
    const QByteArray tag     = QByteArray::fromBase64(tagB64,  QByteArray::Base64UrlEncoding);
    if (json.isEmpty() || tag.isEmpty()) {
        return std::nullopt;
    }
    const QByteArray expected = hmacSha256(key, json);
    if (expected.isEmpty() || expected.size() != tag.size()) {
        return std::nullopt;
    }
    if (CRYPTO_memcmp(expected.constData(), tag.constData(),
                      static_cast<size_t>(expected.size())) != 0) {
        return std::nullopt;
    }
    return deserialize(json);
}

bool HmacLicense::isExpired(const LicensePayload& payload, const QDateTime& now) {
    if (!payload.expires.isValid() || !now.isValid()) {
        return true;
    }
    return payload.expires.toUTC() < now.toUTC();
}
