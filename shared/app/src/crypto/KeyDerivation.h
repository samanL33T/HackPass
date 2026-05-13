#pragma once

#include <QByteArray>
#include <QFuture>

class KeyDerivation {
public:
    enum class Algo {
        Argon2id           = 1,
        PBKDF2_HMAC_SHA256 = 2,
    };

    struct Params {
        Algo algo          = Algo::Argon2id;
        // For Argon2id: time cost.   Reasonable: 3.
        // For PBKDF2:   iteration count. Reasonable: 600000 (OWASP 2024).
        int  iterations    = 3;
        // Argon2id only: memory cost in KiB. 65536 = 64 MiB.
        int  memoryKB      = 65536;
        // Argon2id only: parallelism.
        int  parallelism   = 1;
        // Output key length in bytes. 32 for AES-256.
        int  outputLength  = 32;
    };

    // Synchronous derivation. Returns an empty QByteArray on failure.
    // For Argon2id requests on OpenSSL builds without ARGON2ID support,
    // automatically falls back to PBKDF2-HMAC-SHA256 at the same iteration count
    // (use a higher iteration count for PBKDF2 if calling explicitly).
    static QByteArray derive(const QByteArray& password,
                             const QByteArray& salt,
                             const Params&     params);

    // Async derivation on QThreadPool::globalInstance(). Resolve via QFutureWatcher
    // or qcoro / coroutines.
    static QFuture<QByteArray> deriveAsync(const QByteArray& password,
                                           const QByteArray& salt,
                                           const Params&     params);

    // Returns true if OpenSSL on this build provides Argon2id (OpenSSL 3.2+).
    static bool argon2idAvailable();
};
