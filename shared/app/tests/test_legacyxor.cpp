#include <QtTest>

#include "crypto/LegacyXOR.h"

class LegacyXORTest : public QObject {
    Q_OBJECT
private slots:
    void roundtrip();
    void empty_input();
    void key_extractable();
    void short_input();
    void long_input();
};

void LegacyXORTest::roundtrip() {
    const QByteArray pt = "sync-token-1234567890abcdef";
    const QByteArray ct = LegacyXOR::encrypt(pt);
    QCOMPARE(LegacyXOR::decrypt(ct), pt);
}

void LegacyXORTest::empty_input() {
    QCOMPARE(LegacyXOR::encrypt(QByteArray()), QByteArray());
    QCOMPARE(LegacyXOR::decrypt(QByteArray()), QByteArray());
}

void LegacyXORTest::key_extractable() {
    // The embedded key must be retrievable as a plain QByteArray. The lesson is
    // that this string lives in the binary and a tester finds it via strings(1).
    const QByteArray key = LegacyXOR::embeddedKey();
    QVERIFY(!key.isEmpty());
    QCOMPARE(key, QByteArray("hackpass-legacy-sync-token-key"));
}

void LegacyXORTest::short_input() {
    const QByteArray pt = "a";
    const QByteArray ct = LegacyXOR::encrypt(pt);
    QCOMPARE(ct.size(), 1);
    QCOMPARE(LegacyXOR::decrypt(ct), pt);
}

void LegacyXORTest::long_input() {
    QByteArray pt;
    pt.resize(8192);
    for (int i = 0; i < pt.size(); ++i) {
        pt[i] = static_cast<char>(i & 0xFF);
    }
    const QByteArray ct = LegacyXOR::encrypt(pt);
    QCOMPARE(ct.size(), pt.size());
    QCOMPARE(LegacyXOR::decrypt(ct), pt);
}

QTEST_GUILESS_MAIN(LegacyXORTest)
#include "test_legacyxor.moc"
