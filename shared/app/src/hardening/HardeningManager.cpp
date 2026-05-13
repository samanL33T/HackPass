#include "HardeningManager.h"

#include "core/IDebuggerCheck.h"
#include "core/IIntegrityCheck.h"
#include "core/IPlatformSecurity.h"
#include "core/IProcessScanner.h"
#include "core/Platform.h"
#include "settings/AppSettings.h"
#include "vault/VaultManager.h"

HardeningManager::HardeningManager(AppSettings*  settings,
                                   VaultManager* vaultManager,
                                   QObject*      parent)
    : QObject(parent), m_settings(settings), m_vaultManager(vaultManager) {
    m_pollTimer.setInterval(500);
    connect(&m_pollTimer, &QTimer::timeout, this, &HardeningManager::onPollTimeout);
    if (m_settings) {
        setEnabled(m_settings->hardeningEnabled());
        connect(m_settings, &AppSettings::hardeningEnabledChanged, this, [this]() {
            setEnabled(m_settings->hardeningEnabled());
        });
    }
    if (m_vaultManager) {
        connect(this, &HardeningManager::tamperDetected,
                m_vaultManager, &VaultManager::onTamperDetected);
    }
}

HardeningManager::~HardeningManager() {
    setEnabled(false);
}

bool HardeningManager::isEnabled() const {
    return m_enabled;
}

void HardeningManager::setEnabled(bool v) {
    if (v == m_enabled) return;
    m_enabled = v;
    if (v) {
        rebuildChecks();
        m_pollTimer.start();
    } else {
        m_pollTimer.stop();
        m_debuggerChecks.clear();
        m_processScanners.clear();
        m_integrityChecks.clear();
    }
    emit enabledChanged();
}

bool HardeningManager::checkAtCallsite(QStringView callsiteName) {
    if (!m_enabled) return false;
    QString reason;
    if (anyCheckFires(&reason)) {
        emit tamperDetected(QStringLiteral("inline check at %1: %2")
                                .arg(callsiteName.toString(), reason));
        return true;
    }
    return false;
}

void HardeningManager::onPollTimeout() {
    QString reason;
    if (anyCheckFires(&reason)) {
        emit tamperDetected(QStringLiteral("poll check: %1").arg(reason));
        m_pollTimer.stop();
    }
}

void HardeningManager::rebuildChecks() {
    m_debuggerChecks.clear();
    m_processScanners.clear();
    m_integrityChecks.clear();
    auto& sec = Platform::security();
    if (auto d = sec.makeDebuggerCheck())  m_debuggerChecks.push_back(std::move(d));
    if (auto p = sec.makeProcessScanner()) m_processScanners.push_back(std::move(p));
    if (auto i = sec.makeIntegrityCheck()) m_integrityChecks.push_back(std::move(i));
}

bool HardeningManager::anyCheckFires(QString* reason) {
    for (const auto& d : m_debuggerChecks) {
        if (d->isDebuggerPresent()) {
            if (reason) *reason = QStringLiteral("debugger: %1").arg(d->lastDetectionReason());
            return true;
        }
    }
    for (const auto& s : m_processScanners) {
        if (s->suspiciousProcessRunning()) {
            if (reason) *reason = QStringLiteral("suspect process: %1")
                                       .arg(s->matchedProcessNames().join(QLatin1Char(',')));
            return true;
        }
    }
    for (const auto& i : m_integrityChecks) {
        if (!i->binaryUntampered()) {
            if (reason) *reason = QStringLiteral("integrity: %1").arg(i->lastFailureReason());
            return true;
        }
    }
    return false;
}
