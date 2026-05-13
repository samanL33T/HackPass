#pragma once

#include <QByteArray>
#include <QObject>
#include <QSettings>
#include <QString>

class AppSettings : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString theme            READ theme            WRITE setTheme            NOTIFY themeChanged)
    Q_PROPERTY(int     autoLockMinutes  READ autoLockMinutes  WRITE setAutoLockMinutes  NOTIFY autoLockMinutesChanged)
    Q_PROPERTY(bool    hardeningEnabled READ hardeningEnabled WRITE setHardeningEnabled NOTIFY hardeningEnabledChanged)
    Q_PROPERTY(QString serverUrl        READ serverUrl        WRITE setServerUrl        NOTIFY serverUrlChanged)
    Q_PROPERTY(QString deviceId         READ deviceId         WRITE setDeviceId         NOTIFY deviceIdChanged)
    Q_PROPERTY(QString recentVaultPath  READ recentVaultPath  WRITE setRecentVaultPath  NOTIFY recentVaultPathChanged)

public:
    explicit AppSettings(QObject* parent = nullptr);

    QString  theme() const;
    void     setTheme(const QString& v);

    int      autoLockMinutes() const;
    void     setAutoLockMinutes(int v);

    QString  recentVaultPath() const;
    void     setRecentVaultPath(const QString& v);

    QByteArray windowGeometry() const;
    void       setWindowGeometry(const QByteArray& v);

    bool     hardeningEnabled() const;
    void     setHardeningEnabled(bool v);

    QString  serverUrl() const;
    void     setServerUrl(const QString& v);

    // Sync token stored XOR-obfuscated through LegacyXOR. The accessor handles
    // the obfuscation; on disk this is a single REG_SZ value that any local process
    // can read. That is the lesson.
    Q_INVOKABLE QString syncToken() const;
    Q_INVOKABLE void    setSyncToken(const QString& v);

    Q_INVOKABLE qint64 lastSyncAt() const;
    Q_INVOKABLE void   setLastSyncAt(qint64 ms);

    QString deviceId() const;
    void    setDeviceId(const QString& v);

    Q_INVOKABLE void sync();
    Q_INVOKABLE void clear();

signals:
    void themeChanged();
    void autoLockMinutesChanged();
    void hardeningEnabledChanged();
    void serverUrlChanged();
    void deviceIdChanged();
    void recentVaultPathChanged();

private:
    QSettings m_settings;
};
