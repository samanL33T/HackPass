#include "PasswordGenerator.h"

#include <QByteArray>

#include <openssl/rand.h>

#include <algorithm>
#include <cstdint>

PasswordGenerator::PasswordGenerator(QObject* parent) : QObject(parent) {}

QString PasswordGenerator::generate(int  length,
                                    bool useUppercase,
                                    bool useLowercase,
                                    bool useDigits,
                                    bool useSymbols,
                                    bool avoidAmbiguous) const {
    if (length <= 0)   length = 20;
    if (length > 1024) length = 1024;

    QByteArray pool;
    if (useUppercase) pool += "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    if (useLowercase) pool += "abcdefghijklmnopqrstuvwxyz";
    if (useDigits)    pool += "0123456789";
    if (useSymbols)   pool += "!@#$%^&*()-_=+[]{};:,.<>?";

    if (avoidAmbiguous) {
        // Characters easily confused with each other in many fonts.
        for (const char c : QByteArray("0Oo1lI|`'\"")) {
            pool.replace(QByteArray(1, c), QByteArray());
        }
    }

    if (pool.isEmpty()) return {};

    // Sample uniformly from the pool using rejection sampling to avoid the
    // modulo bias that would result from `byte % pool.size()` when pool size
    // doesn't divide 256 evenly.
    const int poolSize = pool.size();
    const int maxAccept = 256 - (256 % poolSize);

    QString out;
    out.reserve(length);
    QByteArray buf(64, '\0');
    int produced = 0;
    while (produced < length) {
        if (RAND_bytes(reinterpret_cast<unsigned char*>(buf.data()), buf.size()) != 1) {
            return {};
        }
        for (int i = 0; i < buf.size() && produced < length; ++i) {
            const int b = static_cast<unsigned char>(buf.at(i));
            if (b >= maxAccept) continue;  // rejection sampling
            out.append(QChar(static_cast<char16_t>(pool.at(b % poolSize))));
            ++produced;
        }
    }
    return out;
}

int PasswordGenerator::estimateStrength(const QString& password) const {
    if (password.isEmpty()) return 0;
    bool hasUpper = false, hasLower = false, hasDigit = false, hasSymbol = false;
    for (const QChar c : password) {
        if (c.isUpper()) hasUpper = true;
        else if (c.isLower()) hasLower = true;
        else if (c.isDigit()) hasDigit = true;
        else                  hasSymbol = true;
    }
    const int diversity = int(hasUpper) + int(hasLower) + int(hasDigit) + int(hasSymbol);
    const int len = password.size();
    if (len < 8) return 0;                      // very weak
    if (len < 12 && diversity < 3) return 1;    // weak
    if (len < 16 && diversity < 3) return 2;    // ok
    if (len >= 16 && diversity >= 3) return 4;  // strong
    return 3;                                   // good
}
