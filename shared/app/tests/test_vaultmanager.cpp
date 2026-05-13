#include <QtTest>

#include <QSignalSpy>
#include <QTemporaryFile>

#include "vault/VaultManager.h"
#include "vault/VaultModel.h"

class VaultManagerTest : public QObject {
    Q_OBJECT
private slots:
    void initial_state_is_no_vault();
    void create_then_unlock_then_lock();
    void wrong_password_keeps_locked();
    void auto_lock_clears_plaintext();
    void tamper_transitions_to_tampered();
};

void VaultManagerTest::initial_state_is_no_vault() {
    VaultModel m;
    VaultManager v(&m);
    QCOMPARE(v.state(), VaultManager::State::NoVault);
}

void VaultManagerTest::create_then_unlock_then_lock() {
    VaultModel m;
    VaultManager v(&m);
    QTemporaryFile tf;
    QVERIFY(tf.open());
    const QString path = tf.fileName();
    tf.close();
    tf.remove();

    QVERIFY(v.createNew(path, "hackpass"));
    QCOMPARE(v.state(), VaultManager::State::Unlocked);
    v.lock();
    QCOMPARE(v.state(), VaultManager::State::Locked);
    QVERIFY(v.unlock("hackpass"));
    QCOMPARE(v.state(), VaultManager::State::Unlocked);
}

void VaultManagerTest::wrong_password_keeps_locked() {
    VaultModel m;
    VaultManager v(&m);
    QTemporaryFile tf;
    QVERIFY(tf.open());
    const QString path = tf.fileName();
    tf.close();
    tf.remove();

    QVERIFY(v.createNew(path, "good"));
    v.lock();
    QSignalSpy fail(&v, &VaultManager::unlockFailed);
    QVERIFY(!v.unlock("bad"));
    QCOMPARE(v.state(), VaultManager::State::Locked);
    QCOMPARE(fail.size(), 1);
}

void VaultManagerTest::auto_lock_clears_plaintext() {
    VaultModel m;
    VaultManager v(&m);
    QTemporaryFile tf;
    QVERIFY(tf.open());
    const QString path = tf.fileName();
    tf.close();
    tf.remove();

    // createNew leaves the manager already in Unlocked state; no separate
    // unlock call is needed (and would be rejected as a state precondition).
    QVERIFY(v.createNew(path, "pwd"));
    QCOMPARE(v.state(), VaultManager::State::Unlocked);
    m.addEntry(VaultEntry::makeLogin("X", "u", "secret"));
    QCOMPARE(m.rowCount(), 1);
    v.autoLock();
    QCOMPARE(v.state(), VaultManager::State::AutoLocked);
    QCOMPARE(m.rowCount(), 0);
}

void VaultManagerTest::tamper_transitions_to_tampered() {
    VaultModel m;
    VaultManager v(&m);
    QSignalSpy spy(&v, &VaultManager::tamperedShutdown);
    v.onTamperDetected("test");
    QCOMPARE(v.state(), VaultManager::State::Tampered);
    QCOMPARE(spy.size(), 1);
}

QTEST_GUILESS_MAIN(VaultManagerTest)
#include "test_vaultmanager.moc"
