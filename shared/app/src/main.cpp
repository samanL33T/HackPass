#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QGuiApplication>
#include <QIcon>
#include <QProcess>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QSettings>
#include <QStandardPaths>
#include <QTcpSocket>
#include <QThread>
#include <QUrl>
#include <QWindow>

#include "core/Clipboard.h"
#include "core/IDebuggerCheck.h"
#include "core/IPlatformPaths.h"
#include "core/IPlatformSecurity.h"
#include "core/IPlatformUi.h"
#include "core/IProcessScanner.h"
#include "core/Logging.h"
#include "core/Platform.h"

#include "bridge/LicenseGate.h"
#include "bridge/PremiumGate.h"
#include "bridge/ServerFlagsApplier.h"
#include "bridge/SettingsBridge.h"
#include "crypto/PasswordGenerator.h"
#include "crypto/Totp.h"
#include "hardening/HardeningManager.h"
#include "ipc/BrowserBridge.h"
#include "ipc/TokenHandshake.h"
#include "network/LicenseClient.h"
#include "network/MessageClient.h"
#include "network/PinnedNetworkAccessManager.h"
#include "network/PolicyClient.h"
#include "network/SyncClient.h"
#include "network/TofuStore.h"
#include "settings/AppSettings.h"
#include "settings/TokenStore.h"
#include "vault/VaultManager.h"
#include "vault/VaultModel.h"

namespace {

void logPhase1Status() {
    auto& p = Platform::paths();
    auto& s = Platform::security();

    qInfo().noquote() << "HackPass - platform probe";
    qInfo().noquote() << "  settingsScopeKey:      " << p.settingsScopeKey();
    qInfo().noquote() << "  defaultVaultDir:       " << QDir::toNativeSeparators(p.defaultVaultDir());
    qInfo().noquote() << "  configDir:             " << QDir::toNativeSeparators(p.configDir());
    qInfo().noquote() << "  cacheDir:              " << QDir::toNativeSeparators(p.cacheDir());
    qInfo().noquote() << "  ipcEndpointDescriptor: " << p.ipcEndpointDescriptor();
    qInfo().noquote() << "  isCodeSigningValid:    " << s.isCodeSigningValid();
}

// Ensures the bundled backend is running before the app tries to reach it.
// Called early in main so the app works regardless of how it was launched
// (Start Menu shortcut, pinned taskbar icon, direct double-click of
// HackPass.exe). If the configured server_url points at a non-localhost
// host, this is a no-op - user is running their own backend.
//
// Returns a QProcess* the caller should keep alive (parented to the app)
// so the backend dies with us. nullptr if we didn't spawn (backend already
// up, or hackpass-server.exe not bundled).
QProcess* ensureBackend(QObject* parent) {
    QSettings settings(QSettings::NativeFormat, QSettings::UserScope,
                       QStringLiteral("HackPass"), QStringLiteral("app"));
    const QString urlStr = settings.value(QStringLiteral("server_url"),
                                          QStringLiteral("https://localhost:8443")).toString();
    const QUrl url(urlStr);
    const QString host = url.host();
    const int port = url.port(8443);
    const bool isLocal = (host == QLatin1String("localhost") ||
                          host == QLatin1String("127.0.0.1") ||
                          host == QLatin1String("::1"));
    if (!isLocal) {
        qInfo() << "ensureBackend: remote URL configured, not spawning local backend";
        return nullptr;
    }

    QTcpSocket probe;
    probe.connectToHost(QStringLiteral("127.0.0.1"), static_cast<quint16>(port));
    if (probe.waitForConnected(500)) {
        probe.disconnectFromHost();
        qInfo() << "ensureBackend: backend already reachable on port" << port;
        return nullptr;
    }

    // Locate the bundled backend binary.
#if defined(Q_OS_WIN)
    const QString serverExe = QCoreApplication::applicationDirPath() +
                              QStringLiteral("/hackpass-server.exe");
#elif defined(Q_OS_MACOS)
    const QString serverExe = QCoreApplication::applicationDirPath() +
                              QStringLiteral("/../Resources/hackpass-server");
#else
    const QString serverExe = QCoreApplication::applicationDirPath() +
                              QStringLiteral("/hackpass-server");
#endif
    if (!QFileInfo::exists(serverExe)) {
        qWarning() << "ensureBackend: server binary not found at" << serverExe;
        return nullptr;
    }

    // Same dataDir the external launcher uses, so TOFU certs persist.
    QString dataDir;
#if defined(Q_OS_WIN)
    const QString localAppData = QProcessEnvironment::systemEnvironment().value(
        QStringLiteral("LOCALAPPDATA"));
    dataDir = localAppData.isEmpty()
        ? QDir::homePath() + QStringLiteral("/AppData/Local/HackPass/backend-data")
        : QDir(localAppData).filePath(QStringLiteral("HackPass/backend-data"));
#elif defined(Q_OS_MACOS)
    dataDir = QDir::homePath() +
              QStringLiteral("/Library/Application Support/HackPass/backend-data");
#else
    dataDir = QDir::homePath() +
              QStringLiteral("/.local/share/HackPass/backend-data");
#endif
    QDir().mkpath(dataDir);

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("HACKPASS_PORT"),     QString::number(port));
    env.insert(QStringLiteral("HACKPASS_DATA_DIR"), dataDir);

    auto* proc = new QProcess(parent);
    proc->setProcessEnvironment(env);
    proc->setProgram(serverExe);
    // Set WD to the binary's dir so the bundled openssl is on the same path.
    proc->setWorkingDirectory(QFileInfo(serverExe).absolutePath());
    proc->setStandardOutputFile(QDir(dataDir).filePath(QStringLiteral("backend.out.log")),
                                QIODevice::Append);
    proc->setStandardErrorFile(QDir(dataDir).filePath(QStringLiteral("backend.err.log")),
                               QIODevice::Append);
#ifdef Q_OS_WIN
    proc->setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments* args) {
        args->flags |= 0x08000000;  // CREATE_NO_WINDOW
    });
#endif
    proc->start();
    if (!proc->waitForStarted(5000)) {
        qWarning() << "ensureBackend: failed to start backend:" << proc->errorString();
        proc->deleteLater();
        return nullptr;
    }

    QElapsedTimer t; t.start();
    while (t.elapsed() < 15000) {
        QTcpSocket s;
        s.connectToHost(QStringLiteral("127.0.0.1"), static_cast<quint16>(port));
        if (s.waitForConnected(500)) {
            s.disconnectFromHost();
            qInfo() << "ensureBackend: backend up on port" << port;
            return proc;
        }
        if (proc->state() != QProcess::Running) {
            qWarning() << "ensureBackend: backend exited before binding port";
            return nullptr;
        }
        QThread::msleep(250);
    }
    qWarning() << "ensureBackend: backend did not bind port" << port << "within 15s";
    return proc;
}

}  // namespace

int main(int argc, char** argv) {
    QGuiApplication::setApplicationName(QStringLiteral("HackPass"));
    QGuiApplication::setOrganizationName(QStringLiteral("HackPass"));
    QGuiApplication::setApplicationVersion(QStringLiteral("1.0.0-dev"));
    QGuiApplication::setApplicationDisplayName(QStringLiteral("HackPass"));

    QGuiApplication app(argc, argv);

    const QString logFile = QDir(Platform::paths().configDir()).filePath(QStringLiteral("hackpass.log"));
    installFileMessageHandler(logFile);

    // Make sure the backend is running before the app tries to reach it.
    // This covers the case where the user pinned HackPass.exe to taskbar and
    // clicks it directly (bypassing the launcher script). QProcess is parented
    // to `app` so it gets cleaned up when the app exits.
    QProcess* backendProc = ensureBackend(&app);
    Q_UNUSED(backendProc);

    QGuiApplication::setWindowIcon(QIcon(QStringLiteral(":/qt/qml/HackPass/Ui/images/logo.svg")));
    Platform::ui().applyAppStyleHints();

    logPhase1Status();

    // Core services. Owned on the stack; live for the app lifetime.
    Clipboard          clipboard;
    Totp               totp;
    PasswordGenerator  passwordGenerator;
    AppSettings        appSettings;
    TokenStore         tokenStore;
    VaultModel         vaultModel;
    VaultManager       vaultManager(&vaultModel);
    LicenseGate        licenseGate;
    PremiumGate        premiumGate;
    ServerFlagsApplier serverFlagsApplier(&premiumGate, &appSettings);
    SettingsBridge     settingsBridge(&appSettings, &licenseGate, &premiumGate,
                                      &serverFlagsApplier, &vaultManager);
    HardeningManager   hardening(&appSettings, &vaultManager);
    TofuStore          tofuStore;
    PinnedNetworkAccessManager nam(&tofuStore);
    SyncClient         syncClient(&nam, &appSettings);
    LicenseClient      licenseClient(&nam, &appSettings);
    PolicyClient       policyClient(&nam, &appSettings);
    MessageClient      messageClient(&nam, &appSettings);
    TokenHandshake     handshake(&tokenStore);
    BrowserBridge      browserBridge(&vaultManager, &vaultModel, &handshake, &hardening);

    // Wire sync responses into the server-flags applier.
    QObject::connect(&syncClient,   &SyncClient::serverFlagsReceived,
                     &serverFlagsApplier, &ServerFlagsApplier::apply);
    QObject::connect(&policyClient, &PolicyClient::policyReceived,
                     &serverFlagsApplier, &ServerFlagsApplier::apply);

    // On every transition into Unlocked, refresh policy from the backend.
    // Backend tells us whether this device is premium, what server messages
    // to show, etc. Failures here are non-fatal; we just stay with the
    // previous server-flags state.
    QObject::connect(&vaultManager, &VaultManager::stateChanged,
                     [&policyClient](VaultManager::State newState,
                                     VaultManager::State oldState) {
        if (newState == VaultManager::State::Unlocked &&
            oldState != VaultManager::State::Syncing) {
            policyClient.fetch();
        }
    });

    // Bring up the localhost WebSocket for the extension. Failure is non-fatal.
    if (!browserBridge.listen(8765)) {
        qWarning() << "BrowserBridge failed to listen on 127.0.0.1:8765";
    }

    // If a default vault already exists on disk, pre-select it so the LoginPage
    // can transition straight from NoVault -> Locked -> Unlocked. Otherwise the
    // user has to run the first-run wizard.
    const QString defaultVaultPath =
        QDir(Platform::paths().defaultVaultDir()).filePath(QStringLiteral("default.hkps"));
    vaultManager.selectFile(defaultVaultPath);
    QGuiApplication::setApplicationVersion(QGuiApplication::applicationVersion());

    QQmlApplicationEngine engine;
    auto* root = engine.rootContext();
    root->setContextProperty(QStringLiteral("appSettings"),        &appSettings);
    root->setContextProperty(QStringLiteral("tokenStore"),         &tokenStore);
    root->setContextProperty(QStringLiteral("vaultModel"),         &vaultModel);
    root->setContextProperty(QStringLiteral("vaultManager"),       &vaultManager);
    root->setContextProperty(QStringLiteral("licenseGate"),        &licenseGate);
    root->setContextProperty(QStringLiteral("premiumGate"),        &premiumGate);
    root->setContextProperty(QStringLiteral("serverFlagsApplier"), &serverFlagsApplier);
    root->setContextProperty(QStringLiteral("settingsBridge"),     &settingsBridge);
    root->setContextProperty(QStringLiteral("hardening"),          &hardening);
    root->setContextProperty(QStringLiteral("syncClient"),         &syncClient);
    root->setContextProperty(QStringLiteral("licenseClient"),      &licenseClient);
    root->setContextProperty(QStringLiteral("policyClient"),       &policyClient);
    root->setContextProperty(QStringLiteral("messageClient"),      &messageClient);
    root->setContextProperty(QStringLiteral("nam"),                &nam);
    root->setContextProperty(QStringLiteral("tofuStore"),          &tofuStore);
    root->setContextProperty(QStringLiteral("clipboard"),          &clipboard);
    root->setContextProperty(QStringLiteral("totp"),               &totp);
    root->setContextProperty(QStringLiteral("passwordGenerator"),  &passwordGenerator);
    root->setContextProperty(QStringLiteral("defaultVaultPath"),   defaultVaultPath);

    engine.loadFromModule("HackPass.Ui", "Main");
    if (engine.rootObjects().isEmpty()) {
        qCritical() << "Failed to load HackPass.Ui / Main";
        return 1;
    }

    if (auto* window = qobject_cast<QWindow*>(engine.rootObjects().first())) {
        Platform::ui().applyWindowChromeHints(window);
        Platform::ui().registerGlobalHotkeys(window);
    }

    return QGuiApplication::exec();
}
