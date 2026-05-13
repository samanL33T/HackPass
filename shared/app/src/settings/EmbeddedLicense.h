#pragma once

#include <QByteArray>
#include <QString>
#include <optional>

#include "crypto/HmacLicense.h"

// Reads the qrc-bundled evaluation license. The HMAC key is intentionally
// embedded as a plain string constant a tester can find with strings(1).
class EmbeddedLicense {
public:
    // Loads the qrc resource at qrc:/qt/qml/HackPass/Ui/eval_license.bin and
    // verifies its HMAC signature with the embedded key. Returns std::nullopt
    // if the resource is missing or verification fails.
    static std::optional<LicensePayload> load();

    // Returns the embedded HMAC key. Public for tests and so tampering with the
    // qrc payload is detectable. The lesson is: this key sits in .rdata.
    static QByteArray embeddedKey();

    // Convenience for tests and dev tooling.
    static QByteArray mint(const LicensePayload& payload);
};
