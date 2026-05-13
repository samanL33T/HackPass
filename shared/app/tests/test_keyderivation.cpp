#include <QtTest>

#include "crypto/KeyDerivation.h"

class KeyDerivationTest : public QObject {
    Q_OBJECT
private slots:
    void rfc6070_pbkdf2_vector1();
    void rfc6070_pbkdf2_vector2();
    void empty_inputs_return_empty();
    void argon2id_roundtrip_when_available();
    void async_returns_same_as_sync();
};

void KeyDerivationTest::rfc6070_pbkdf2_vector1() {
    // RFC 6070 PBKDF2-HMAC-SHA1 vectors are SHA-1 based; OpenSSL EVP_KDF supports
    // arbitrary digests. We use an analogous SHA-256 vector for the same inputs.
    // Test: P="password", S="salt", c=1, dkLen=32 with HMAC-SHA256.
    KeyDerivation::Params p;
    p.algo         = KeyDerivation::Algo::PBKDF2_HMAC_SHA256;
    p.iterations   = 1;
    p.outputLength = 32;

    const QByteArray key = KeyDerivation::derive("password", "salt", p);
    QCOMPARE(key.size(), 32);
    // Known answer for PBKDF2-HMAC-SHA256("password", "salt", 1, 32):
    //   120fb6cffcf8b32c43e7225256c4f837a86548c92ccc35480805987cb70be17b
    const QByteArray expected = QByteArray::fromHex(
        "120fb6cffcf8b32c43e7225256c4f837a86548c92ccc35480805987cb70be17b");
    QCOMPARE(key.toHex(), expected.toHex());
}

void KeyDerivationTest::rfc6070_pbkdf2_vector2() {
    // PBKDF2-HMAC-SHA256("password", "salt", 4096, 32):
    //   c5e478d59288c841aa530db6845c4c8d962893a001ce4e11a4963873aa98134a
    KeyDerivation::Params p;
    p.algo         = KeyDerivation::Algo::PBKDF2_HMAC_SHA256;
    p.iterations   = 4096;
    p.outputLength = 32;

    const QByteArray key = KeyDerivation::derive("password", "salt", p);
    QCOMPARE(key.size(), 32);
    const QByteArray expected = QByteArray::fromHex(
        "c5e478d59288c841aa530db6845c4c8d962893a001ce4e11a4963873aa98134a");
    QCOMPARE(key.toHex(), expected.toHex());
}

void KeyDerivationTest::empty_inputs_return_empty() {
    KeyDerivation::Params p;
    QVERIFY(KeyDerivation::derive(QByteArray(), "salt",          p).isEmpty());
    QVERIFY(KeyDerivation::derive("password",   QByteArray(),     p).isEmpty());
    p.outputLength = 0;
    QVERIFY(KeyDerivation::derive("password",   "salt",           p).isEmpty());
}

void KeyDerivationTest::argon2id_roundtrip_when_available() {
    if (!KeyDerivation::argon2idAvailable()) {
        QSKIP("OpenSSL build does not provide Argon2id; fallback path covered by other tests");
    }
    KeyDerivation::Params p;
    p.algo         = KeyDerivation::Algo::Argon2id;
    p.iterations   = 2;
    p.memoryKB     = 4096;
    p.parallelism  = 1;
    p.outputLength = 32;

    const QByteArray salt(16, char(0x42));
    const QByteArray a = KeyDerivation::derive("hackpass", salt, p);
    const QByteArray b = KeyDerivation::derive("hackpass", salt, p);
    QCOMPARE(a.size(), 32);
    QCOMPARE(a.toHex(), b.toHex());
    // Different salt produces different key.
    const QByteArray salt2(16, char(0x43));
    const QByteArray c = KeyDerivation::derive("hackpass", salt2, p);
    QVERIFY(a.toHex() != c.toHex());
}

void KeyDerivationTest::async_returns_same_as_sync() {
    KeyDerivation::Params p;
    p.algo         = KeyDerivation::Algo::PBKDF2_HMAC_SHA256;
    p.iterations   = 1000;
    p.outputLength = 32;

    auto f = KeyDerivation::deriveAsync("password", "salt", p);
    f.waitForFinished();
    const QByteArray async = f.result();
    const QByteArray sync  = KeyDerivation::derive("password", "salt", p);
    QCOMPARE(async.toHex(), sync.toHex());
}

QTEST_GUILESS_MAIN(KeyDerivationTest)
#include "test_keyderivation.moc"
