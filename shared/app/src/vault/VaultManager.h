#pragma once

#include <QByteArray>
#include <QDateTime>
#include <QObject>
#include <QString>
#include <QTimer>

#include "VaultEntry.h"
#include "VaultFile.h"

class VaultModel;

class VaultManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(State state READ state NOTIFY stateChanged)

public:
    enum class State {
        NoVault    = 0,
        Locked     = 1,
        Unlocked   = 2,
        AutoLocked = 3,
        Syncing    = 4,
        Tampered   = 5,
    };
    Q_ENUM(State)

    explicit VaultManager(VaultModel* model, QObject* parent = nullptr);
    ~VaultManager() override;

    Q_INVOKABLE int     stateValue() const { return static_cast<int>(m_state); }
    State               state() const { return m_state; }
    Q_INVOKABLE QString vaultPath() const { return m_vaultPath; }

    Q_INVOKABLE bool selectFile(const QString& path);
    Q_INVOKABLE bool unlock(const QString& masterPassword);
    Q_INVOKABLE bool reUnlockAfterAutoLock(const QString& masterPassword);
    Q_INVOKABLE bool save();
    Q_INVOKABLE void lock();
    Q_INVOKABLE void autoLock();
    Q_INVOKABLE void close();
    Q_INVOKABLE bool createNew(const QString& path, const QString& masterPassword);

    // Begin a sync transaction. Transitions Unlocked -> Syncing. Caller must call
    // syncDone or syncFailed.
    bool beginSync();
    void syncDone();
    void syncFailed();

    // Triggered by hardening layer. Zeros state, transitions to Tampered.
    void onTamperDetected(const QString& reason);

    // Inactivity tracking. Reset on user activity; fires autoLock when threshold elapsed.
    void touchActivity();
    void setAutoLockMinutes(int minutes);
    int  autoLockMinutes() const { return m_autoLockMinutes; }

    // Reads the file header so the caller can show metadata (createdMs etc.)
    // without unlocking.
    std::optional<VaultFile::Header> peekHeader() const;

signals:
    void stateChanged(VaultManager::State newState, VaultManager::State oldState);
    void tamperedShutdown(QString reason);
    void unlockFailed();

private slots:
    void onAutoLockTimerFired();

private:
    void setState(State s);
    void zeroPlaintext();
    static QByteArray readAll(const QString& path);
    static bool       writeAll(const QString& path, const QByteArray& bytes);

    VaultModel* m_model = nullptr;
    State       m_state = State::NoVault;
    QString     m_vaultPath;
    QByteArray  m_derivedKey;
    VaultFile::Header m_currentHeader;
    QDateTime   m_lastActivity;
    int         m_autoLockMinutes = 5;
    QTimer      m_autoLockTimer;
};
