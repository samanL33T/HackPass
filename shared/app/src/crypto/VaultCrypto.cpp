#include "VaultCrypto.h"

#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#include <memory>

namespace {

struct EvpCipherCtxDeleter {
    void operator()(EVP_CIPHER_CTX* p) const noexcept { if (p) EVP_CIPHER_CTX_free(p); }
};
using EvpCipherCtxPtr = std::unique_ptr<EVP_CIPHER_CTX, EvpCipherCtxDeleter>;

}  // namespace

std::optional<VaultCrypto::Sealed>
VaultCrypto::encrypt(const QByteArray& plaintext,
                     const QByteArray& key,
                     const QByteArray& iv,
                     const QByteArray& aad) {
    if (key.size() != KEY_LENGTH || iv.size() != IV_LENGTH) {
        return std::nullopt;
    }

    EvpCipherCtxPtr ctx(EVP_CIPHER_CTX_new());
    if (!ctx) {
        return std::nullopt;
    }

    if (EVP_EncryptInit_ex(ctx.get(), EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) {
        return std::nullopt;
    }
    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_IVLEN, IV_LENGTH, nullptr) != 1) {
        return std::nullopt;
    }
    if (EVP_EncryptInit_ex(ctx.get(), nullptr, nullptr,
                           reinterpret_cast<const unsigned char*>(key.constData()),
                           reinterpret_cast<const unsigned char*>(iv.constData())) != 1) {
        return std::nullopt;
    }

    if (!aad.isEmpty()) {
        int aadOut = 0;
        if (EVP_EncryptUpdate(ctx.get(), nullptr, &aadOut,
                              reinterpret_cast<const unsigned char*>(aad.constData()),
                              aad.size()) != 1) {
            return std::nullopt;
        }
    }

    QByteArray ciphertext(plaintext.size(), '\0');
    int outLen = 0;
    if (EVP_EncryptUpdate(ctx.get(),
                          reinterpret_cast<unsigned char*>(ciphertext.data()),
                          &outLen,
                          reinterpret_cast<const unsigned char*>(plaintext.constData()),
                          plaintext.size()) != 1) {
        return std::nullopt;
    }

    int finalLen = 0;
    if (EVP_EncryptFinal_ex(ctx.get(),
                            reinterpret_cast<unsigned char*>(ciphertext.data()) + outLen,
                            &finalLen) != 1) {
        return std::nullopt;
    }
    ciphertext.resize(outLen + finalLen);

    QByteArray tag(TAG_LENGTH, '\0');
    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_GET_TAG, TAG_LENGTH, tag.data()) != 1) {
        return std::nullopt;
    }

    return Sealed{std::move(ciphertext), std::move(tag)};
}

std::optional<QByteArray>
VaultCrypto::decrypt(const QByteArray& ciphertext,
                     const QByteArray& tag,
                     const QByteArray& key,
                     const QByteArray& iv,
                     const QByteArray& aad) {
    if (key.size() != KEY_LENGTH || iv.size() != IV_LENGTH || tag.size() != TAG_LENGTH) {
        return std::nullopt;
    }

    EvpCipherCtxPtr ctx(EVP_CIPHER_CTX_new());
    if (!ctx) {
        return std::nullopt;
    }

    if (EVP_DecryptInit_ex(ctx.get(), EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) {
        return std::nullopt;
    }
    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_IVLEN, IV_LENGTH, nullptr) != 1) {
        return std::nullopt;
    }
    if (EVP_DecryptInit_ex(ctx.get(), nullptr, nullptr,
                           reinterpret_cast<const unsigned char*>(key.constData()),
                           reinterpret_cast<const unsigned char*>(iv.constData())) != 1) {
        return std::nullopt;
    }

    if (!aad.isEmpty()) {
        int aadOut = 0;
        if (EVP_DecryptUpdate(ctx.get(), nullptr, &aadOut,
                              reinterpret_cast<const unsigned char*>(aad.constData()),
                              aad.size()) != 1) {
            return std::nullopt;
        }
    }

    QByteArray plaintext(ciphertext.size(), '\0');
    int outLen = 0;
    if (EVP_DecryptUpdate(ctx.get(),
                          reinterpret_cast<unsigned char*>(plaintext.data()),
                          &outLen,
                          reinterpret_cast<const unsigned char*>(ciphertext.constData()),
                          ciphertext.size()) != 1) {
        return std::nullopt;
    }

    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_TAG, TAG_LENGTH,
                            const_cast<char*>(tag.constData())) != 1) {
        return std::nullopt;
    }

    int finalLen = 0;
    if (EVP_DecryptFinal_ex(ctx.get(),
                            reinterpret_cast<unsigned char*>(plaintext.data()) + outLen,
                            &finalLen) != 1) {
        // Tag mismatch or other auth failure. OpenSSL handles this in constant time.
        return std::nullopt;
    }
    plaintext.resize(outLen + finalLen);

    return plaintext;
}

QByteArray VaultCrypto::randomIv() {
    QByteArray iv(IV_LENGTH, '\0');
    if (RAND_bytes(reinterpret_cast<unsigned char*>(iv.data()), IV_LENGTH) != 1) {
        return {};
    }
    return iv;
}

QByteArray VaultCrypto::randomKey() {
    QByteArray key(KEY_LENGTH, '\0');
    if (RAND_bytes(reinterpret_cast<unsigned char*>(key.data()), KEY_LENGTH) != 1) {
        return {};
    }
    return key;
}
