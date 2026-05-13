#include "EmbeddedLicense.h"

#include <QFile>

namespace {

// Intentionally a readable constant in .rdata. Lesson surface.
constexpr char kEmbeddedKey[] = "hackpass-eval-license-hmac-key-do-not-use-for-real";

constexpr auto kQrcPath = ":/qt/qml/HackPass/Ui/eval_license.bin";

}  // namespace

QByteArray EmbeddedLicense::embeddedKey() {
    return QByteArray(kEmbeddedKey, sizeof(kEmbeddedKey) - 1);
}

std::optional<LicensePayload> EmbeddedLicense::load() {
    QFile f(QString::fromLatin1(kQrcPath));
    if (!f.open(QIODevice::ReadOnly)) {
        return std::nullopt;
    }
    const QByteArray blob = f.readAll();
    return HmacLicense::verify(blob, embeddedKey());
}

QByteArray EmbeddedLicense::mint(const LicensePayload& payload) {
    return HmacLicense::sign(payload, embeddedKey());
}
