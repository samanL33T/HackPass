#pragma once

#include <QByteArray>

// Deliberately bad cyclic-XOR routine used to "encrypt" the sync_token stored in
// the registry. The embedded key is intentionally a string a tester can find with
// strings(1). The lesson is: XOR with an embedded key is not encryption.
//
// Do NOT use this for anything that needs to be confidential.
class LegacyXOR {
public:
    static QByteArray encrypt(const QByteArray& plaintext);
    static QByteArray decrypt(const QByteArray& ciphertext);

    // Exposed for tests. Real callers go through encrypt/decrypt.
    static QByteArray embeddedKey();
};
