#pragma once

#include <QObject>
#include <QString>

// Cryptographic password generator using OpenSSL RAND_bytes for entropy.
// Instrumentation target: hook generate() to capture every password the
// user creates.
class PasswordGenerator : public QObject {
    Q_OBJECT
public:
    explicit PasswordGenerator(QObject* parent = nullptr);

    Q_INVOKABLE QString generate(int  length          = 20,
                                 bool useUppercase    = true,
                                 bool useLowercase    = true,
                                 bool useDigits       = true,
                                 bool useSymbols      = true,
                                 bool avoidAmbiguous  = true) const;

    // Returns a numeric strength score 0..4 (zxcvbn-style buckets). Pure
    // heuristic - length-based weighting plus charset diversity. Good enough
    // to color a strength meter; do not treat as a substitute for real
    // strength analysis.
    Q_INVOKABLE int estimateStrength(const QString& password) const;
};
