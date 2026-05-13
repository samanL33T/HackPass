#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <memory>
#include <vector>

class IDebuggerCheck;
class IProcessScanner;
class IIntegrityCheck;
class AppSettings;
class VaultManager;

// HardeningManager runs a worker-thread polling timer when the toggle is on,
// AND exposes a checkAtCallsite hook called inline from five named call sites
// (VaultManager::unlock, SyncClient::pull, BrowserBridge::onAuthenticate,
// MainWindow::showEvent, SettingsBridge::setPremium). Multi-point design forces
// an attacker to hook every entry, not just the polling loop.
class HardeningManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)

public:
    HardeningManager(AppSettings*  settings,
                     VaultManager* vaultManager,
                     QObject*      parent = nullptr);
    ~HardeningManager() override;

    bool isEnabled() const;
    void setEnabled(bool v);

    // Called inline from the five protected call sites. Returns true if
    // tampering is detected; caller MUST refuse to proceed in that case.
    bool checkAtCallsite(QStringView callsiteName);

signals:
    void enabledChanged();
    void tamperDetected(QString reason);

private slots:
    void onPollTimeout();

private:
    void rebuildChecks();
    bool anyCheckFires(QString* reason);

    AppSettings*  m_settings    = nullptr;
    VaultManager* m_vaultManager = nullptr;

    std::vector<std::unique_ptr<IDebuggerCheck>>  m_debuggerChecks;
    std::vector<std::unique_ptr<IProcessScanner>> m_processScanners;
    std::vector<std::unique_ptr<IIntegrityCheck>> m_integrityChecks;

    QTimer  m_pollTimer;
    bool    m_enabled = false;
};
