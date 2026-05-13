#pragma once

#include <QByteArray>
#include <optional>

class VaultCrypto {
public:
    static constexpr int KEY_LENGTH = 32;  // AES-256
    static constexpr int IV_LENGTH  = 12;  // GCM recommended IV length
    static constexpr int TAG_LENGTH = 16;  // GCM tag

    struct Sealed {
        QByteArray ciphertext;
        QByteArray tag;  // exactly TAG_LENGTH bytes
    };

    // Encrypt with AES-256-GCM. Returns std::nullopt on parameter or OpenSSL failure.
    // Caller MUST provide a fresh, unique iv per encryption with the same key.
    // aad is optional Additional Authenticated Data: covered by the tag but not encrypted.
    static std::optional<Sealed> encrypt(const QByteArray& plaintext,
                                         const QByteArray& key,
                                         const QByteArray& iv,
                                         const QByteArray& aad = {});

    // Decrypt with AES-256-GCM. Returns std::nullopt on parameter mismatch or tag failure.
    // Constant-time tag verification is handled by OpenSSL's EVP_DecryptFinal_ex.
    static std::optional<QByteArray> decrypt(const QByteArray& ciphertext,
                                             const QByteArray& tag,
                                             const QByteArray& key,
                                             const QByteArray& iv,
                                             const QByteArray& aad = {});

    // Returns IV_LENGTH cryptographically random bytes via OpenSSL's RAND_bytes.
    // Returns empty on RAND failure (rare; caller should treat as fatal).
    static QByteArray randomIv();

    // Returns KEY_LENGTH cryptographically random bytes. Use only for ephemeral keys;
    // long-lived keys should come from KeyDerivation.
    static QByteArray randomKey();
};
