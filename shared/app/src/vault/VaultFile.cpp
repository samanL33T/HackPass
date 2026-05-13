#include "VaultFile.h"

#include "crypto/KeyDerivation.h"
#include "crypto/VaultCrypto.h"

#include <QDataStream>
#include <QDateTime>
#include <QIODevice>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>

#include <openssl/rand.h>

namespace {

QByteArray randomBytes(int n) {
    QByteArray out(n, '\0');
    if (RAND_bytes(reinterpret_cast<unsigned char*>(out.data()), n) != 1) {
        return {};
    }
    return out;
}

QByteArray serializePayload(const VaultFile::Payload& p) {
    QJsonObject o;
    o["schema_version"] = p.schemaVersion.isEmpty() ? QStringLiteral("1") : p.schemaVersion;
    o["device_id"]      = p.deviceId;
    o["vault_id"]       = p.vaultId;
    o["vault_name"]     = p.vaultName;

    QJsonArray items;
    for (const auto& e : p.items) {
        items.append(e.toJson());
    }
    o["items"] = items;

    if (!p.rawMetadataJson.isEmpty()) {
        QJsonParseError err{};
        const auto doc = QJsonDocument::fromJson(p.rawMetadataJson, &err);
        if (err.error == QJsonParseError::NoError && doc.isObject()) {
            o["metadata"] = doc.object();
        }
    }
    return QJsonDocument(o).toJson(QJsonDocument::Compact);
}

std::optional<VaultFile::Payload> parsePayload(const QByteArray& json) {
    QJsonParseError err{};
    const auto doc = QJsonDocument::fromJson(json, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        return std::nullopt;
    }
    const auto o = doc.object();
    VaultFile::Payload p;
    p.schemaVersion = o.value("schema_version").toString();
    p.deviceId      = o.value("device_id").toString();
    p.vaultId       = o.value("vault_id").toString();
    p.vaultName     = o.value("vault_name").toString();
    for (const auto& v : o.value("items").toArray()) {
        p.items.append(VaultEntry::fromJson(v.toObject()));
    }
    if (o.contains("metadata") && o.value("metadata").isObject()) {
        p.rawMetadataJson = QJsonDocument(o.value("metadata").toObject()).toJson(QJsonDocument::Compact);
    }
    return p;
}

}  // namespace

QByteArray VaultFile::deriveKey(const Header& h, const QByteArray& password) {
    KeyDerivation::Params kd;
    kd.outputLength = VaultCrypto::KEY_LENGTH;
    if (h.kdfType == KdfType::Argon2id) {
        kd.algo        = KeyDerivation::Algo::Argon2id;
        kd.iterations  = static_cast<int>(h.kdfIterations);
        kd.memoryKB    = static_cast<int>(h.kdfMemoryKB);
        kd.parallelism = static_cast<int>(h.kdfParallel);
    } else {
        kd.algo       = KeyDerivation::Algo::PBKDF2_HMAC_SHA256;
        kd.iterations = static_cast<int>(h.kdfIterations);
    }
    return KeyDerivation::derive(password, h.salt, kd);
}

VaultFile::Header VaultFile::makeHeader() {
    Header h;
    h.salt      = randomBytes(SALT_SIZE);
    h.iv        = randomBytes(IV_SIZE);
    h.createdMs = QDateTime::currentMSecsSinceEpoch();
    return h;
}

QByteArray VaultFile::writeHeader(const Header& h) {
    QByteArray buf;
    buf.reserve(HEADER_SIZE);
    QDataStream s(&buf, QIODevice::WriteOnly);
    s.setByteOrder(QDataStream::LittleEndian);
    s << MAGIC;
    s << h.version;
    s << quint16(0);
    s << static_cast<quint32>(h.kdfType);
    s << h.kdfIterations;
    s << h.kdfMemoryKB;
    s << h.kdfParallel;
    s.writeRawData(h.salt.constData(), SALT_SIZE);
    s.writeRawData(h.iv.constData(),   IV_SIZE);
    s << quint32(0);
    s << h.createdMs;
    Q_ASSERT(buf.size() == HEADER_SIZE);
    return buf;
}

std::optional<VaultFile::Header> VaultFile::readHeader(const QByteArray& bytes) {
    if (bytes.size() < HEADER_SIZE) return std::nullopt;
    QDataStream s(bytes.left(HEADER_SIZE));
    s.setByteOrder(QDataStream::LittleEndian);

    Header h;
    quint32 magic      = 0;
    quint16 reserved16 = 0;
    quint32 kdfType    = 0;
    quint32 reserved32 = 0;
    s >> magic;
    if (magic != MAGIC) return std::nullopt;
    s >> h.version;
    if (h.version != VERSION) return std::nullopt;
    s >> reserved16;
    s >> kdfType;
    if (kdfType != quint32(KdfType::Argon2id) && kdfType != quint32(KdfType::Pbkdf2HmacSha256)) {
        return std::nullopt;
    }
    h.kdfType = static_cast<KdfType>(kdfType);
    s >> h.kdfIterations;
    s >> h.kdfMemoryKB;
    s >> h.kdfParallel;
    h.salt.resize(SALT_SIZE);
    h.iv.resize(IV_SIZE);
    if (s.readRawData(h.salt.data(), SALT_SIZE) != SALT_SIZE) return std::nullopt;
    if (s.readRawData(h.iv.data(),   IV_SIZE)   != IV_SIZE)   return std::nullopt;
    s >> reserved32;
    s >> h.createdMs;
    return h;
}

std::optional<QByteArray> VaultFile::saveWithKey(const Payload&    payload,
                                                 const Header&     header,
                                                 const QByteArray& key) {
    if (header.salt.size() != SALT_SIZE || header.iv.size() != IV_SIZE) {
        return std::nullopt;
    }
    if (key.size() != VaultCrypto::KEY_LENGTH) {
        return std::nullopt;
    }
    const QByteArray plaintext   = serializePayload(payload);
    const QByteArray headerBytes = writeHeader(header);
    auto sealed = VaultCrypto::encrypt(plaintext, key, header.iv, headerBytes);
    if (!sealed.has_value()) {
        return std::nullopt;
    }
    QByteArray out;
    out.reserve(headerBytes.size() + sealed->ciphertext.size() + TAG_SIZE);
    out.append(headerBytes);
    out.append(sealed->ciphertext);
    out.append(sealed->tag);
    return out;
}

std::optional<VaultFile::Payload> VaultFile::loadWithKey(const QByteArray& fileBytes,
                                                         const QByteArray& key,
                                                         Header*           outHeader) {
    auto headerOpt = readHeader(fileBytes);
    if (!headerOpt.has_value()) return std::nullopt;
    const Header& h = *headerOpt;
    if (fileBytes.size() < HEADER_SIZE + TAG_SIZE) return std::nullopt;
    if (key.size() != VaultCrypto::KEY_LENGTH)     return std::nullopt;

    const QByteArray ciphertext  = fileBytes.mid(HEADER_SIZE, fileBytes.size() - HEADER_SIZE - TAG_SIZE);
    const QByteArray tag         = fileBytes.right(TAG_SIZE);
    const QByteArray headerBytes = fileBytes.left(HEADER_SIZE);
    auto plaintext = VaultCrypto::decrypt(ciphertext, tag, key, h.iv, headerBytes);
    if (!plaintext.has_value()) return std::nullopt;
    auto parsed = parsePayload(*plaintext);
    if (!parsed.has_value()) return std::nullopt;
    if (outHeader) *outHeader = h;
    return parsed;
}

std::optional<QByteArray> VaultFile::save(const Payload&    payload,
                                          const Header&     header,
                                          const QByteArray& password) {
    const QByteArray key = deriveKey(header, password);
    return saveWithKey(payload, header, key);
}

std::optional<VaultFile::Payload> VaultFile::load(const QByteArray& fileBytes,
                                                  const QByteArray& password,
                                                  Header*           outHeader) {
    auto headerOpt = readHeader(fileBytes);
    if (!headerOpt.has_value()) return std::nullopt;
    const QByteArray key = deriveKey(*headerOpt, password);
    if (key.size() != VaultCrypto::KEY_LENGTH) return std::nullopt;
    return loadWithKey(fileBytes, key, outHeader);
}
