#include <QtTest>

#include "crypto/HmacLicense.h"

class HmacLicenseTest : public QObject {
    Q_OBJECT
private slots:
    void sign_and_verify_roundtrip();
    void verify_with_wrong_key_fails();
    void verify_with_tampered_payload_fails();
    void verify_with_tampered_tag_fails();
    void rejects_malformed_blob();
    void rejects_empty_inputs();
    void expiry_check();
};

void HmacLicenseTest::sign_and_verify_roundtrip() {
    LicensePayload p;
    p.user    = "samanl33t";
    p.tier    = "pro";
    p.expires = QDateTime(QDate(2030, 1, 1), QTime(0, 0), QTimeZone(QTimeZone::UTC));

    const QByteArray key  = "license-hmac-key-test";
    const QByteArray blob = HmacLicense::sign(p, key);
    QVERIFY(!blob.isEmpty());

    auto recovered = HmacLicense::verify(blob, key);
    QVERIFY(recovered.has_value());
    QCOMPARE(recovered->user, p.user);
    QCOMPARE(recovered->tier, p.tier);
    QCOMPARE(recovered->expires.toUTC(), p.expires.toUTC());
}

void HmacLicenseTest::verify_with_wrong_key_fails() {
    LicensePayload p;
    p.user    = "samanl33t";
    p.tier    = "pro";
    p.expires = QDateTime(QDate(2030, 1, 1), QTime(0, 0), QTimeZone(QTimeZone::UTC));

    const QByteArray blob = HmacLicense::sign(p, "right-key");
    QVERIFY(!HmacLicense::verify(blob, "wrong-key").has_value());
}

void HmacLicenseTest::verify_with_tampered_payload_fails() {
    LicensePayload p;
    p.user    = "samanl33t";
    p.tier    = "free";
    p.expires = QDateTime(QDate(2030, 1, 1), QTime(0, 0), QTimeZone(QTimeZone::UTC));

    const QByteArray key  = "license-hmac-key-test";
    QByteArray blob = HmacLicense::sign(p, key);
    // Flip a bit in the payload portion (before the '.').
    const int dot = blob.indexOf('.');
    QVERIFY(dot > 0);
    blob[0] = static_cast<char>(blob[0] ^ 1);
    QVERIFY(!HmacLicense::verify(blob, key).has_value());
}

void HmacLicenseTest::verify_with_tampered_tag_fails() {
    LicensePayload p;
    p.user    = "samanl33t";
    p.tier    = "free";
    p.expires = QDateTime(QDate(2030, 1, 1), QTime(0, 0), QTimeZone(QTimeZone::UTC));

    const QByteArray key  = "license-hmac-key-test";
    QByteArray blob = HmacLicense::sign(p, key);
    const int dot = blob.indexOf('.');
    QVERIFY(dot > 0);
    blob[dot + 1] = static_cast<char>(blob[dot + 1] ^ 1);
    QVERIFY(!HmacLicense::verify(blob, key).has_value());
}

void HmacLicenseTest::rejects_malformed_blob() {
    const QByteArray key = "k";
    QVERIFY(!HmacLicense::verify("",          key).has_value());
    QVERIFY(!HmacLicense::verify("no-dot",    key).has_value());
    QVERIFY(!HmacLicense::verify(".no-payload", key).has_value());
    QVERIFY(!HmacLicense::verify("no-tag.",  key).has_value());
}

void HmacLicenseTest::rejects_empty_inputs() {
    LicensePayload empty;
    QVERIFY(HmacLicense::sign(empty, "key").isEmpty());
    LicensePayload p;
    p.user    = "u";
    p.tier    = "t";
    p.expires = QDateTime(QDate(2030, 1, 1), QTime(0, 0), QTimeZone(QTimeZone::UTC));
    QVERIFY(HmacLicense::sign(p, QByteArray()).isEmpty());
}

void HmacLicenseTest::expiry_check() {
    LicensePayload p;
    p.user    = "u";
    p.tier    = "free";
    p.expires = QDateTime(QDate(2020, 1, 1), QTime(0, 0), QTimeZone(QTimeZone::UTC));
    QVERIFY( HmacLicense::isExpired(p, QDateTime(QDate(2026, 1, 1), QTime(0, 0), QTimeZone(QTimeZone::UTC))));
    QVERIFY(!HmacLicense::isExpired(p, QDateTime(QDate(2019, 1, 1), QTime(0, 0), QTimeZone(QTimeZone::UTC))));
}

QTEST_GUILESS_MAIN(HmacLicenseTest)
#include "test_hmaclicense.moc"
