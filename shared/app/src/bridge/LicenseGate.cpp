#include "LicenseGate.h"

#include "settings/EmbeddedLicense.h"
#include "crypto/HmacLicense.h"

LicenseGate::LicenseGate(QObject* parent) : QObject(parent) {
    refresh();
}

bool LicenseGate::isLicenseValid() const {
    return m_valid;
}

void LicenseGate::refresh() {
    const auto payload = EmbeddedLicense::load();
    const bool newValid = payload.has_value() && !HmacLicense::isExpired(*payload);
    if (newValid != m_valid) {
        m_valid = newValid;
        emit licenseValidChanged();
    }
}
