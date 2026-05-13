#pragma once

#include <QObject>
#include <QString>

// RFC 6238 TOTP code generator. SHA-1 only (the universal default), 30-second
// period, 6-digit codes. Instrumentation target: hook generateCode to log every
// time a TOTP is produced - this is the secrets-in-memory + secrets-in-flight
// hook surface combined.
class Totp : public QObject {
    Q_OBJECT
public:
    explicit Totp(QObject* parent = nullptr);

    // Generates the current 6-digit TOTP code for the given base32 secret.
    // epochMs of 0 means "use current system time". Returns "------" on
    // malformed input or HMAC failure (never throws).
    Q_INVOKABLE QString generateCode(const QString& base32Secret, qint64 epochMs = 0) const;

    // Seconds remaining in the current 30-second window. UI uses this to drive
    // a countdown / refresh timer.
    Q_INVOKABLE int secondsRemaining(qint64 epochMs = 0) const;
};
