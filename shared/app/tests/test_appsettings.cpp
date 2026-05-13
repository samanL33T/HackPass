#include <QtTest>

#include "settings/AppSettings.h"

class AppSettingsTest : public QObject {
    Q_OBJECT
private slots:
    void initTestCase();
    void roundtrip_simple_values();
    void sync_token_obfuscation();
    void cleanupTestCase();
};

void AppSettingsTest::initTestCase() {
    QCoreApplication::setOrganizationName("HackPass-tests");
    QCoreApplication::setApplicationName("HackPass-tests");
    AppSettings s;
    s.clear();
}

void AppSettingsTest::roundtrip_simple_values() {
    AppSettings s;
    s.setTheme("light");
    s.setAutoLockMinutes(15);
    s.setHardeningEnabled(true);
    s.setServerUrl("https://example.invalid:8443");
    s.setDeviceId("test-device-uuid");
    s.sync();

    AppSettings s2;
    QCOMPARE(s2.theme(),            QStringLiteral("light"));
    QCOMPARE(s2.autoLockMinutes(),  15);
    QCOMPARE(s2.hardeningEnabled(), true);
    QCOMPARE(s2.serverUrl(),        QStringLiteral("https://example.invalid:8443"));
    QCOMPARE(s2.deviceId(),         QStringLiteral("test-device-uuid"));
}

void AppSettingsTest::sync_token_obfuscation() {
    AppSettings s;
    const QString tok = "device-token-abcdef1234567890";
    s.setSyncToken(tok);
    s.sync();

    AppSettings s2;
    QCOMPARE(s2.syncToken(), tok);
}

void AppSettingsTest::cleanupTestCase() {
    AppSettings s;
    s.clear();
}

QTEST_GUILESS_MAIN(AppSettingsTest)
#include "test_appsettings.moc"
