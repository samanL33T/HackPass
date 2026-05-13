#include "LegacyXOR.h"

namespace {

// Intentionally a plaintext constant in .rdata, recoverable by strings(1).
// Part of the teaching surface for the storage-at-rest lesson.
constexpr char kEmbeddedKey[] = "hackpass-legacy-sync-token-key";

QByteArray xorCycle(const QByteArray& input) {
    if (input.isEmpty()) {
        return {};
    }
    const QByteArray key(kEmbeddedKey, sizeof(kEmbeddedKey) - 1);
    const int keyLen = key.size();
    QByteArray out(input.size(), Qt::Uninitialized);
    for (int i = 0; i < input.size(); ++i) {
        out[i] = static_cast<char>(input.at(i) ^ key.at(i % keyLen));
    }
    return out;
}

}  // namespace

QByteArray LegacyXOR::encrypt(const QByteArray& plaintext)  { return xorCycle(plaintext); }
QByteArray LegacyXOR::decrypt(const QByteArray& ciphertext) { return xorCycle(ciphertext); }

QByteArray LegacyXOR::embeddedKey() {
    return QByteArray(kEmbeddedKey, sizeof(kEmbeddedKey) - 1);
}
