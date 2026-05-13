#pragma once

#include <QByteArray>
#include <QObject>
#include <QSettings>
#include <QString>

// Trust-on-first-use store for HTTPS server cert fingerprints. Persists to
// HKCU\Software\HackPass\tofu so a local attacker with write access can
// pre-poison entries before the user first connects (intentional lesson).
class TofuStore : public QObject {
    Q_OBJECT
public:
    explicit TofuStore(QObject* parent = nullptr);

    // Returns the stored SHA-256 of the cert public key, or empty if unknown.
    QByteArray fingerprint(const QString& host) const;

    // Stores the SHA-256 fingerprint for host. Overwrites any previous value.
    void       setFingerprint(const QString& host, const QByteArray& sha256);

    // Removes a host's entry.
    void       forget(const QString& host);

    // Constant-time compare host's stored fingerprint against candidate.
    // Returns false if host is unknown OR fingerprints differ.
    bool       matches(const QString& host, const QByteArray& candidateSha256) const;

private:
    QSettings m_store;
};
