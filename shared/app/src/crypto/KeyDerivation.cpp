#include "KeyDerivation.h"

#include <QtConcurrent/QtConcurrent>

#include <openssl/core_names.h>
#include <openssl/evp.h>
#include <openssl/kdf.h>
#include <openssl/params.h>

#include <memory>

namespace {

struct EvpKdfDeleter {
    void operator()(EVP_KDF* p) const noexcept { if (p) EVP_KDF_free(p); }
};
struct EvpKdfCtxDeleter {
    void operator()(EVP_KDF_CTX* p) const noexcept { if (p) EVP_KDF_CTX_free(p); }
};
using EvpKdfPtr    = std::unique_ptr<EVP_KDF,     EvpKdfDeleter>;
using EvpKdfCtxPtr = std::unique_ptr<EVP_KDF_CTX, EvpKdfCtxDeleter>;

QByteArray deriveArgon2id(const QByteArray& password,
                          const QByteArray& salt,
                          const KeyDerivation::Params& params) {
    EvpKdfPtr kdf(EVP_KDF_fetch(nullptr, "ARGON2ID", nullptr));
    if (!kdf) {
        return {};
    }
    EvpKdfCtxPtr ctx(EVP_KDF_CTX_new(kdf.get()));
    if (!ctx) {
        return {};
    }

    uint32_t iterations  = static_cast<uint32_t>(params.iterations);
    uint32_t memoryKB    = static_cast<uint32_t>(params.memoryKB);
    uint32_t parallelism = static_cast<uint32_t>(params.parallelism);

    // Use string literals for the argon2-specific keys so the code compiles
    // against OpenSSL 3.0 and 3.1 (where the OSSL_KDF_PARAM_ARGON2_* macros
    // are not defined). When ARGON2ID is unavailable, EVP_KDF_fetch returns
    // null above and we never reach here.
    OSSL_PARAM pset[] = {
        OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_PASSWORD,
                                          const_cast<char*>(password.constData()),
                                          static_cast<size_t>(password.size())),
        OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_SALT,
                                          const_cast<char*>(salt.constData()),
                                          static_cast<size_t>(salt.size())),
        OSSL_PARAM_construct_uint32(OSSL_KDF_PARAM_ITER,    &iterations),
        OSSL_PARAM_construct_uint32(OSSL_KDF_PARAM_THREADS, &parallelism),
        OSSL_PARAM_construct_uint32("lanes",    &parallelism),
        OSSL_PARAM_construct_uint32("memcost",  &memoryKB),
        OSSL_PARAM_construct_end(),
    };

    QByteArray out(params.outputLength, '\0');
    const int rc = EVP_KDF_derive(ctx.get(),
                                  reinterpret_cast<unsigned char*>(out.data()),
                                  static_cast<size_t>(out.size()),
                                  pset);
    if (rc <= 0) {
        return {};
    }
    return out;
}

QByteArray derivePbkdf2(const QByteArray& password,
                        const QByteArray& salt,
                        const KeyDerivation::Params& params) {
    EvpKdfPtr kdf(EVP_KDF_fetch(nullptr, "PBKDF2", nullptr));
    if (!kdf) {
        return {};
    }
    EvpKdfCtxPtr ctx(EVP_KDF_CTX_new(kdf.get()));
    if (!ctx) {
        return {};
    }

    char digest[] = "SHA256";
    int  iter     = params.iterations;

    OSSL_PARAM pset[] = {
        OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_PASSWORD,
                                          const_cast<char*>(password.constData()),
                                          static_cast<size_t>(password.size())),
        OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_SALT,
                                          const_cast<char*>(salt.constData()),
                                          static_cast<size_t>(salt.size())),
        OSSL_PARAM_construct_int(OSSL_KDF_PARAM_ITER, &iter),
        OSSL_PARAM_construct_utf8_string(OSSL_KDF_PARAM_DIGEST, digest, 0),
        OSSL_PARAM_construct_end(),
    };

    QByteArray out(params.outputLength, '\0');
    const int rc = EVP_KDF_derive(ctx.get(),
                                  reinterpret_cast<unsigned char*>(out.data()),
                                  static_cast<size_t>(out.size()),
                                  pset);
    if (rc <= 0) {
        return {};
    }
    return out;
}

}  // namespace

QByteArray KeyDerivation::derive(const QByteArray& password,
                                 const QByteArray& salt,
                                 const Params&     params) {
    if (password.isEmpty() || salt.isEmpty() || params.outputLength <= 0) {
        return {};
    }

    if (params.algo == Algo::Argon2id) {
        if (auto k = deriveArgon2id(password, salt, params); !k.isEmpty()) {
            return k;
        }
        // Argon2id unavailable on this OpenSSL build. Fall back to PBKDF2.
        Params fb        = params;
        fb.algo          = Algo::PBKDF2_HMAC_SHA256;
        if (fb.iterations < 100000) {
            fb.iterations = 600000;
        }
        return derivePbkdf2(password, salt, fb);
    }
    return derivePbkdf2(password, salt, params);
}

QFuture<QByteArray> KeyDerivation::deriveAsync(const QByteArray& password,
                                               const QByteArray& salt,
                                               const Params&     params) {
    return QtConcurrent::run([password, salt, params]() {
        return KeyDerivation::derive(password, salt, params);
    });
}

bool KeyDerivation::argon2idAvailable() {
    EvpKdfPtr kdf(EVP_KDF_fetch(nullptr, "ARGON2ID", nullptr));
    return kdf != nullptr;
}
