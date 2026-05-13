#include <QtTest>

#include "crypto/VaultCrypto.h"

class VaultCryptoTest : public QObject {
    Q_OBJECT
private slots:
    void roundtrip_simple();
    void roundtrip_with_aad();
    void roundtrip_empty_plaintext();
    void roundtrip_large_plaintext();
    void rejects_wrong_key_size();
    void rejects_wrong_iv_size();
    void rejects_tampered_ciphertext();
    void rejects_tampered_tag();
    void rejects_tampered_aad();
    void rejects_wrong_key();
    void random_iv_unique();
    void nist_gcm_vector();
};

void VaultCryptoTest::roundtrip_simple() {
    const QByteArray key(VaultCrypto::KEY_LENGTH, char(0x10));
    const QByteArray iv(VaultCrypto::IV_LENGTH,   char(0x20));
    const QByteArray pt = "the vault is open";
    auto sealed = VaultCrypto::encrypt(pt, key, iv);
    QVERIFY(sealed.has_value());
    QCOMPARE(sealed->tag.size(), VaultCrypto::TAG_LENGTH);

    auto recovered = VaultCrypto::decrypt(sealed->ciphertext, sealed->tag, key, iv);
    QVERIFY(recovered.has_value());
    QCOMPARE(*recovered, pt);
}

void VaultCryptoTest::roundtrip_with_aad() {
    const QByteArray key(VaultCrypto::KEY_LENGTH, char(0x11));
    const QByteArray iv(VaultCrypto::IV_LENGTH,   char(0x22));
    const QByteArray pt  = "secret payload";
    const QByteArray aad = "version=1,device=abc";
    auto sealed = VaultCrypto::encrypt(pt, key, iv, aad);
    QVERIFY(sealed.has_value());

    auto recovered = VaultCrypto::decrypt(sealed->ciphertext, sealed->tag, key, iv, aad);
    QVERIFY(recovered.has_value());
    QCOMPARE(*recovered, pt);
}

void VaultCryptoTest::roundtrip_empty_plaintext() {
    const QByteArray key(VaultCrypto::KEY_LENGTH, char(0x12));
    const QByteArray iv(VaultCrypto::IV_LENGTH,   char(0x23));
    auto sealed = VaultCrypto::encrypt(QByteArray(), key, iv);
    QVERIFY(sealed.has_value());
    QCOMPARE(sealed->ciphertext.size(), 0);
    QCOMPARE(sealed->tag.size(), VaultCrypto::TAG_LENGTH);

    auto recovered = VaultCrypto::decrypt(sealed->ciphertext, sealed->tag, key, iv);
    QVERIFY(recovered.has_value());
    QCOMPARE(recovered->size(), 0);
}

void VaultCryptoTest::roundtrip_large_plaintext() {
    const QByteArray key(VaultCrypto::KEY_LENGTH, char(0x13));
    const QByteArray iv(VaultCrypto::IV_LENGTH,   char(0x24));
    QByteArray pt;
    pt.resize(1 << 20);  // 1 MiB
    for (int i = 0; i < pt.size(); ++i) {
        pt[i] = static_cast<char>(i & 0xFF);
    }
    auto sealed = VaultCrypto::encrypt(pt, key, iv);
    QVERIFY(sealed.has_value());
    QCOMPARE(sealed->ciphertext.size(), pt.size());

    auto recovered = VaultCrypto::decrypt(sealed->ciphertext, sealed->tag, key, iv);
    QVERIFY(recovered.has_value());
    QCOMPARE(*recovered, pt);
}

void VaultCryptoTest::rejects_wrong_key_size() {
    const QByteArray badKey(16, char(0x14));
    const QByteArray iv(VaultCrypto::IV_LENGTH, char(0x25));
    QVERIFY(!VaultCrypto::encrypt("x", badKey, iv).has_value());
    QVERIFY(!VaultCrypto::decrypt("x", QByteArray(16, '\0'), badKey, iv).has_value());
}

void VaultCryptoTest::rejects_wrong_iv_size() {
    const QByteArray key(VaultCrypto::KEY_LENGTH, char(0x15));
    const QByteArray badIv(8, char(0x26));
    QVERIFY(!VaultCrypto::encrypt("x", key, badIv).has_value());
    QVERIFY(!VaultCrypto::decrypt("x", QByteArray(16, '\0'), key, badIv).has_value());
}

void VaultCryptoTest::rejects_tampered_ciphertext() {
    const QByteArray key(VaultCrypto::KEY_LENGTH, char(0x16));
    const QByteArray iv(VaultCrypto::IV_LENGTH,   char(0x27));
    auto sealed = VaultCrypto::encrypt("hello", key, iv);
    QVERIFY(sealed.has_value());
    QByteArray bad = sealed->ciphertext;
    bad[0] = static_cast<char>(bad[0] ^ 1);
    QVERIFY(!VaultCrypto::decrypt(bad, sealed->tag, key, iv).has_value());
}

void VaultCryptoTest::rejects_tampered_tag() {
    const QByteArray key(VaultCrypto::KEY_LENGTH, char(0x17));
    const QByteArray iv(VaultCrypto::IV_LENGTH,   char(0x28));
    auto sealed = VaultCrypto::encrypt("hello", key, iv);
    QVERIFY(sealed.has_value());
    QByteArray badTag = sealed->tag;
    badTag[0] = static_cast<char>(badTag[0] ^ 1);
    QVERIFY(!VaultCrypto::decrypt(sealed->ciphertext, badTag, key, iv).has_value());
}

void VaultCryptoTest::rejects_tampered_aad() {
    const QByteArray key(VaultCrypto::KEY_LENGTH, char(0x18));
    const QByteArray iv(VaultCrypto::IV_LENGTH,   char(0x29));
    auto sealed = VaultCrypto::encrypt("hello", key, iv, "aad-a");
    QVERIFY(sealed.has_value());
    QVERIFY(!VaultCrypto::decrypt(sealed->ciphertext, sealed->tag, key, iv, "aad-b").has_value());
}

void VaultCryptoTest::rejects_wrong_key() {
    const QByteArray key1(VaultCrypto::KEY_LENGTH, char(0x19));
    const QByteArray key2(VaultCrypto::KEY_LENGTH, char(0x1A));
    const QByteArray iv(VaultCrypto::IV_LENGTH,    char(0x2A));
    auto sealed = VaultCrypto::encrypt("hello", key1, iv);
    QVERIFY(sealed.has_value());
    QVERIFY(!VaultCrypto::decrypt(sealed->ciphertext, sealed->tag, key2, iv).has_value());
}

void VaultCryptoTest::random_iv_unique() {
    QSet<QByteArray> seen;
    for (int i = 0; i < 64; ++i) {
        const QByteArray iv = VaultCrypto::randomIv();
        QCOMPARE(iv.size(), VaultCrypto::IV_LENGTH);
        QVERIFY(!seen.contains(iv));
        seen.insert(iv);
    }
}

void VaultCryptoTest::nist_gcm_vector() {
    // NIST CAVS GCM test vector, gcmEncryptExtIV256.rsp, Count = 0:
    //   [Keylen = 256][IVlen = 96][PTlen = 0][AADlen = 0][Taglen = 128]
    //   Key  = b52c505a37d78eda5dd34f20c22540ea1b58963cf8e5bf8ffa85f9f2492505b4
    //   IV   = 516c33929df5a3284ff463d7
    //   PT   = (empty)
    //   AAD  = (empty)
    //   CT   = (empty)
    //   Tag  = bdc1ac884d332457a1d2664f168c76f0
    const QByteArray key = QByteArray::fromHex(
        "b52c505a37d78eda5dd34f20c22540ea1b58963cf8e5bf8ffa85f9f2492505b4");
    const QByteArray iv  = QByteArray::fromHex("516c33929df5a3284ff463d7");
    auto sealed = VaultCrypto::encrypt(QByteArray(), key, iv);
    QVERIFY(sealed.has_value());
    QCOMPARE(sealed->ciphertext.size(), 0);
    QCOMPARE(sealed->tag.toHex(),
             QByteArray("bdc1ac884d332457a1d2664f168c76f0"));
}

QTEST_GUILESS_MAIN(VaultCryptoTest)
#include "test_vaultcrypto.moc"
