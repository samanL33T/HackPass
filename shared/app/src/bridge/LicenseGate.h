#pragma once

#include <QObject>

class EmbeddedLicense;

// Bool-returning MOC-routed getter the QML layer queries to gate premium-only
// features. Instrumentation target: hook isLicenseValid to flip true unconditionally.
class LicenseGate : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool licenseValid READ isLicenseValid NOTIFY licenseValidChanged)

public:
    explicit LicenseGate(QObject* parent = nullptr);

    Q_INVOKABLE bool isLicenseValid() const;

    // Cache the result of EmbeddedLicense::load on construction and on rotateRefresh.
    Q_INVOKABLE void refresh();

signals:
    void licenseValidChanged();

private:
    bool m_valid = false;
};
