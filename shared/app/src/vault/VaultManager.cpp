#include "VaultManager.h"

#include "VaultModel.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

VaultManager::VaultManager(VaultModel* model, QObject* parent)
    : QObject(parent), m_model(model) {
    m_autoLockTimer.setSingleShot(true);
    connect(&m_autoLockTimer, &QTimer::timeout, this, &VaultManager::onAutoLockTimerFired);
}

VaultManager::~VaultManager() {
    zeroPlaintext();
}

bool VaultManager::selectFile(const QString& path) {
    m_vaultPath = path;
    if (QFile::exists(path)) {
        setState(State::Locked);
        return true;
    }
    setState(State::NoVault);
    return false;
}

bool VaultManager::unlock(const QString& masterPassword) {
    if (m_state != State::Locked && m_state != State::AutoLocked) {
        return false;
    }
    const QByteArray bytes = readAll(m_vaultPath);
    if (bytes.isEmpty()) {
        emit unlockFailed();
        return false;
    }
    auto headerOpt = VaultFile::readHeader(bytes);
    if (!headerOpt.has_value()) {
        emit unlockFailed();
        return false;
    }
    const QByteArray key = VaultFile::deriveKey(*headerOpt, masterPassword.toUtf8());
    VaultFile::Header header;
    auto payload = VaultFile::loadWithKey(bytes, key, &header);
    if (!payload.has_value()) {
        emit unlockFailed();
        return false;
    }
    if (m_model) {
        m_model->replaceAll(payload->items);
    }
    m_currentHeader = header;
    m_derivedKey    = key;
    touchActivity();
    setState(State::Unlocked);
    return true;
}

bool VaultManager::reUnlockAfterAutoLock(const QString& masterPassword) {
    if (m_state != State::AutoLocked) return false;
    return unlock(masterPassword);
}

bool VaultManager::save() {
    if (m_state != State::Unlocked && m_state != State::Syncing) {
        return false;
    }
    if (!m_model) return false;
    if (m_derivedKey.isEmpty()) return false;

    VaultFile::Payload p;
    p.schemaVersion = QStringLiteral("1");
    p.vaultId       = QStringLiteral("default");
    p.items         = m_model->entries();
    auto bytes = VaultFile::saveWithKey(p, m_currentHeader, m_derivedKey);
    if (!bytes.has_value()) return false;
    return writeAll(m_vaultPath, *bytes);
}

void VaultManager::lock() {
    zeroPlaintext();
    if (!m_vaultPath.isEmpty()) {
        setState(State::Locked);
    } else {
        setState(State::NoVault);
    }
}

void VaultManager::autoLock() {
    zeroPlaintext();
    if (!m_vaultPath.isEmpty()) {
        setState(State::AutoLocked);
    } else {
        setState(State::NoVault);
    }
}

void VaultManager::close() {
    zeroPlaintext();
    m_vaultPath.clear();
    setState(State::NoVault);
}

bool VaultManager::beginSync() {
    if (m_state != State::Unlocked) return false;
    setState(State::Syncing);
    return true;
}

void VaultManager::syncDone() {
    if (m_state == State::Syncing) setState(State::Unlocked);
}

void VaultManager::syncFailed() {
    if (m_state == State::Syncing) setState(State::Unlocked);
}

void VaultManager::onTamperDetected(const QString& reason) {
    zeroPlaintext();
    setState(State::Tampered);
    emit tamperedShutdown(reason);
}

void VaultManager::touchActivity() {
    m_lastActivity = QDateTime::currentDateTimeUtc();
    if (m_autoLockMinutes > 0) {
        m_autoLockTimer.start(m_autoLockMinutes * 60 * 1000);
    } else {
        m_autoLockTimer.stop();
    }
}

void VaultManager::setAutoLockMinutes(int minutes) {
    m_autoLockMinutes = minutes;
    if (m_state == State::Unlocked) touchActivity();
}

std::optional<VaultFile::Header> VaultManager::peekHeader() const {
    if (m_vaultPath.isEmpty()) return std::nullopt;
    const QByteArray bytes = readAll(m_vaultPath);
    if (bytes.isEmpty()) return std::nullopt;
    return VaultFile::readHeader(bytes);
}

bool VaultManager::createNew(const QString& path, const QString& masterPassword) {
    const QFileInfo info(path);
    QDir().mkpath(info.absolutePath());

    // If a vault already exists at this path, back it up by renaming to .bak
    // before we overwrite. Any prior .bak is removed first so this is a
    // single-step rotation, not a chain of suffixes.
    if (QFile::exists(path)) {
        const QString backupPath = path + QStringLiteral(".bak");
        if (QFile::exists(backupPath)) QFile::remove(backupPath);
        if (!QFile::rename(path, backupPath)) {
            qWarning() << "VaultManager::createNew could not back up existing file at" << path;
            return false;
        }
    }

    // Tear down any in-memory state from a previous unlock so the new vault
    // starts clean (model cleared, derived key wiped, autolock timer stopped).
    zeroPlaintext();

    VaultFile::Payload empty;
    empty.schemaVersion = QStringLiteral("1");
    empty.vaultId       = QStringLiteral("default");
    auto header = VaultFile::makeHeader();
    const QByteArray key = VaultFile::deriveKey(header, masterPassword.toUtf8());
    if (key.isEmpty()) return false;
    auto bytes = VaultFile::saveWithKey(empty, header, key);
    if (!bytes.has_value()) return false;
    if (!writeAll(path, *bytes)) return false;
    m_vaultPath     = path;
    m_currentHeader = header;
    m_derivedKey    = key;
    if (m_model) m_model->clear();
    setState(State::Unlocked);
    touchActivity();
    return true;
}

void VaultManager::onAutoLockTimerFired() {
    if (m_state == State::Unlocked) autoLock();
}

void VaultManager::setState(State s) {
    if (s == m_state) return;
    const State old = m_state;
    m_state = s;
    emit stateChanged(s, old);
}

void VaultManager::zeroPlaintext() {
    if (m_model) m_model->zeroAll();
    if (!m_derivedKey.isEmpty()) {
        m_derivedKey.fill(0);
        m_derivedKey.clear();
    }
    m_autoLockTimer.stop();
}

QByteArray VaultManager::readAll(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return {};
    return f.readAll();
}

bool VaultManager::writeAll(const QString& path, const QByteArray& bytes) {
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    const qint64 written = f.write(bytes);
    f.close();
    return written == bytes.size();
}
