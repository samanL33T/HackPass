#pragma once

#include <QObject>

// Bool-returning MOC-routed getter the QML layer queries to gate premium-only
// features. Distinct from LicenseGate so a tester can find either as a hook
// point. Instrumentation target.
class PremiumGate : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool premium READ isPremium WRITE setPremium NOTIFY premiumChanged)

public:
    explicit PremiumGate(QObject* parent = nullptr);

    Q_INVOKABLE bool isPremium() const;
    Q_INVOKABLE void setPremium(bool v);

signals:
    void premiumChanged();

private:
    bool m_premium = false;
};
