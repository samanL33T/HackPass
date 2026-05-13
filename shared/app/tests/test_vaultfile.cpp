#include <QtTest>

#include "vault/VaultEntry.h"
#include "vault/VaultFile.h"

class VaultFileTest : public QObject {
    Q_OBJECT
private slots:
    void header_roundtrip();
    void save_then_load_roundtrip();
    void wrong_password_fails();
    void tampered_ciphertext_fails();
    void tampered_header_fails();
    void invalid_magic_fails();
};

void VaultFileTest::header_roundtrip() {
    auto h    = VaultFile::makeHeader();
    h.kdfType = VaultFile::KdfType::Pbkdf2HmacSha256;
    h.kdfIterations = 1000;
    h.kdfMemoryKB = 0;
    h.kdfParallel = 0;
    QCOMPARE(h.salt.size(), VaultFile::SALT_SIZE);
    QCOMPARE(h.iv.size(),   VaultFile::IV_SIZE);

    const QByteArray bytes = VaultFile::writeHeader(h);
    QCOMPARE(bytes.size(), VaultFile::HEADER_SIZE);

    auto parsed = VaultFile::readHeader(bytes);
    QVERIFY(parsed.has_value());
    QCOMPARE(parsed->salt,          h.salt);
    QCOMPARE(parsed->iv,            h.iv);
    QCOMPARE(parsed->kdfIterations, h.kdfIterations);
    QCOMPARE(parsed->kdfMemoryKB,   h.kdfMemoryKB);
    QCOMPARE(parsed->kdfParallel,   h.kdfParallel);
}

void VaultFileTest::save_then_load_roundtrip() {
    auto h = VaultFile::makeHeader();
    h.kdfType = VaultFile::KdfType::Pbkdf2HmacSha256;
    h.kdfIterations = 1000;

    VaultFile::Payload p;
    p.schemaVersion = "1";
    p.vaultId       = "default";
    p.vaultName     = "Personal";

    VaultEntry e = VaultEntry::makeLogin("Gmail", "user@example.com", "p@ssw0rd");
    e.url = "https://mail.google.com";
    e.tags.append("personal");
    p.items.append(e);

    auto bytes = VaultFile::save(p, h, "hackpass");
    QVERIFY(bytes.has_value());
    QVERIFY(bytes->size() > VaultFile::HEADER_SIZE + VaultFile::TAG_SIZE);

    auto loaded = VaultFile::load(*bytes, "hackpass");
    QVERIFY(loaded.has_value());
    QCOMPARE(loaded->vaultName, p.vaultName);
    QCOMPARE(loaded->items.size(), 1);
    QCOMPARE(loaded->items.at(0).title, QStringLiteral("Gmail"));
    QCOMPARE(loaded->items.at(0).password, QStringLiteral("p@ssw0rd"));
}

void VaultFileTest::wrong_password_fails() {
    auto h = VaultFile::makeHeader();
    h.kdfType = VaultFile::KdfType::Pbkdf2HmacSha256;
    h.kdfIterations = 1000;

    VaultFile::Payload p;
    p.items.append(VaultEntry::makeLogin("X", "u", "v"));

    auto bytes = VaultFile::save(p, h, "right");
    QVERIFY(bytes.has_value());
    QVERIFY(!VaultFile::load(*bytes, "wrong").has_value());
}

void VaultFileTest::tampered_ciphertext_fails() {
    auto h = VaultFile::makeHeader();
    h.kdfType = VaultFile::KdfType::Pbkdf2HmacSha256;
    h.kdfIterations = 1000;
    VaultFile::Payload p;
    p.items.append(VaultEntry::makeLogin("X", "u", "v"));
    auto bytes = VaultFile::save(p, h, "pwd");
    QVERIFY(bytes.has_value());
    (*bytes)[VaultFile::HEADER_SIZE] = static_cast<char>((*bytes)[VaultFile::HEADER_SIZE] ^ 0x01);
    QVERIFY(!VaultFile::load(*bytes, "pwd").has_value());
}

void VaultFileTest::tampered_header_fails() {
    auto h = VaultFile::makeHeader();
    h.kdfType = VaultFile::KdfType::Pbkdf2HmacSha256;
    h.kdfIterations = 1000;
    VaultFile::Payload p;
    auto bytes = VaultFile::save(p, h, "pwd");
    QVERIFY(bytes.has_value());
    (*bytes)[12] = static_cast<char>((*bytes)[12] ^ 0xFF);  // mutate kdfIterations
    QVERIFY(!VaultFile::load(*bytes, "pwd").has_value());
}

void VaultFileTest::invalid_magic_fails() {
    QByteArray junk(VaultFile::HEADER_SIZE + VaultFile::TAG_SIZE, '\0');
    QVERIFY(!VaultFile::load(junk, "pwd").has_value());
    QVERIFY(!VaultFile::readHeader(junk).has_value());
}

QTEST_GUILESS_MAIN(VaultFileTest)
#include "test_vaultfile.moc"
