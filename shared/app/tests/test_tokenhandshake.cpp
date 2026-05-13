#include <QtTest>

#include "ipc/TokenHandshake.h"
#include "settings/TokenStore.h"

class TokenHandshakeTest : public QObject {
    Q_OBJECT
private slots:
    void initTestCase();
    void empty_inputs_fail();
    void short_inputs_fail();
    void exact_match_passes();
    void only_first_16_chars_compared();
    void cleanupTestCase();
};

void TokenHandshakeTest::initTestCase() {
    QCoreApplication::setOrganizationName("HackPass-tests");
    QCoreApplication::setApplicationName("HackPass-tests");
    TokenStore s;
    s.clear();
}

void TokenHandshakeTest::empty_inputs_fail() {
    TokenStore s;
    s.clear();
    TokenHandshake h(&s);
    QVERIFY(!h.validate(""));
    QVERIFY(!h.validate("anything"));
}

void TokenHandshakeTest::short_inputs_fail() {
    TokenStore s;
    s.setToken("01234567890123456789012345678901");  // 32 chars
    TokenHandshake h(&s);
    QVERIFY(!h.validate("short"));      // received too short
    QVERIFY(!h.validate("0123456789abcdef"));  // stored mismatch in first 16
}

void TokenHandshakeTest::exact_match_passes() {
    TokenStore s;
    s.setToken("aaaaaaaaaaaaaaaa-bbbbbbbbbbbbbbbb");
    TokenHandshake h(&s);
    QVERIFY(h.validate("aaaaaaaaaaaaaaaa-bbbbbbbbbbbbbbbb"));
}

void TokenHandshakeTest::only_first_16_chars_compared() {
    TokenStore s;
    s.setToken("aaaaaaaaaaaaaaaa-stored-rest-xxxx");
    TokenHandshake h(&s);
    // Last 16 chars differ but first 16 match. The flaw: this should not pass
    // for a properly-implemented handshake, but our implementation deliberately
    // does. This test PROVES the lesson.
    QVERIFY(h.validate("aaaaaaaaaaaaaaaa-different-yyyyy"));
}

void TokenHandshakeTest::cleanupTestCase() {
    TokenStore s;
    s.clear();
}

QTEST_GUILESS_MAIN(TokenHandshakeTest)
#include "test_tokenhandshake.moc"
