#pragma once

#include <QByteArray>
#include <QList>
#include <QString>
#include <optional>

#include "VaultEntry.h"

// HKPS binary vault file format.
//
// Header layout (64 bytes total):
//   offset  size  field
//   0       4     magic         "HKPS"
//   4       2     version       (uint16_t, currently 1)
//   6       2     reserved      (zero)
//   8       4     kdfType       1=Argon2id, 2=PBKDF2-HMAC-SHA256
//   12      4     kdfIterations time cost (argon2) or iteration count (pbkdf2)
//   16      4     kdfMemoryKB   argon2 memory cost in KiB, 0 for pbkdf2
//   20      4     kdfParallel   argon2 parallelism, 0 for pbkdf2
//   24      16    salt
//   40      12    iv            AES-GCM 96-bit IV
//   52      4     reserved      (zero)
//   56      8     createdMs     unix milliseconds since epoch
//   64...   N     ciphertext
//   64+N    16    tag           AES-GCM tag
//
// Ciphertext plaintext is JSON: { schema_version, device_id, vault_id, items[], metadata }

class VaultFile {
public:
    static constexpr quint32 MAGIC          = 0x53504B48;  // 'HKPS' little-endian
    static constexpr quint16 VERSION        = 1;
    static constexpr int     HEADER_SIZE    = 64;
    static constexpr int     SALT_SIZE      = 16;
    static constexpr int     IV_SIZE        = 12;
    static constexpr int     TAG_SIZE       = 16;

    enum class KdfType : quint32 {
        Argon2id = 1,
        Pbkdf2HmacSha256 = 2,
    };

    struct Header {
        quint16    version       = VERSION;
        KdfType    kdfType       = KdfType::Argon2id;
        quint32    kdfIterations = 3;
        quint32    kdfMemoryKB   = 65536;
        quint32    kdfParallel   = 1;
        QByteArray salt;          // SALT_SIZE bytes
        QByteArray iv;            // IV_SIZE bytes
        qint64     createdMs     = 0;
    };

    struct Payload {
        QString             schemaVersion;
        QString             deviceId;
        QString             vaultId;
        QString             vaultName;
        QList<VaultEntry>   items;
        QByteArray          rawMetadataJson;
    };

    // Password-based entry points: derive the key, then call the WithKey variants.
    static std::optional<QByteArray> save(const Payload&    payload,
                                          const Header&     header,
                                          const QByteArray& password);
    static std::optional<Payload>    load(const QByteArray& fileBytes,
                                          const QByteArray& password,
                                          Header*           outHeader = nullptr);

    // Key-based entry points. Caller has already derived the 32-byte AES key.
    // VaultManager uses these after caching the derived key on unlock so save()
    // does not require the master password at write time.
    static std::optional<QByteArray> saveWithKey(const Payload&    payload,
                                                 const Header&     header,
                                                 const QByteArray& key);
    static std::optional<Payload>    loadWithKey(const QByteArray& fileBytes,
                                                 const QByteArray& key,
                                                 Header*           outHeader = nullptr);

    // Derive the AES key for the given header. Exposed so VaultManager can cache it.
    static QByteArray deriveKey(const Header& header, const QByteArray& password);

    static std::optional<Header> readHeader(const QByteArray& fileBytes);
    static QByteArray            writeHeader(const Header& header);
    static Header                makeHeader();
};
