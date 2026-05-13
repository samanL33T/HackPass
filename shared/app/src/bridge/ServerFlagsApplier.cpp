#include "ServerFlagsApplier.h"

#include "PremiumGate.h"
#include "settings/AppSettings.h"

ServerFlagsApplier::ServerFlagsApplier(PremiumGate* premiumGate, AppSettings* settings, QObject* parent)
    : QObject(parent), m_premiumGate(premiumGate), m_settings(settings) {}

void ServerFlagsApplier::apply(const QVariantMap& flags) {
    // Backend's authoritative "this device is premium" flag. When set, drive
    // PremiumGate directly so the rest of the UI sees premium == true without
    // any user toggle. This is the legitimate path; the lesson-side bypass
    // is the SettingsBridge::setPremium QML hook.
    if (flags.contains("premium_active") && m_premiumGate) {
        m_premiumGate->setPremium(flags.value("premium_active").toBool());
    }
    if (flags.contains("feature_export_plaintext")) {
        const bool v = flags.value("feature_export_plaintext").toBool();
        if (v != m_exportPlaintextAllowed) {
            m_exportPlaintextAllowed = v;
            emit exportPlaintextAllowedChanged();
        }
    }
    if (flags.contains("feature_legacy_kdf_allowed")) {
        const bool v = flags.value("feature_legacy_kdf_allowed").toBool();
        if (v != m_legacyKdfAllowed) {
            m_legacyKdfAllowed = v;
            emit legacyKdfAllowedChanged();
        }
    }
    if (flags.contains("force_relock_required")) {
        const bool v = flags.value("force_relock_required").toBool();
        if (v != m_forceRelockRequired) {
            m_forceRelockRequired = v;
            emit forceRelockRequiredChanged();
        }
    }
    if (flags.contains("policy_message")) {
        const QString v = flags.value("policy_message").toString();
        if (v != m_policyMessage) {
            m_policyMessage = v;
            emit policyMessageChanged();
        }
    }
    if (flags.contains("auto_lock_minutes") && m_settings) {
        const int v = flags.value("auto_lock_minutes").toInt();
        if (v > 0) {
            m_settings->setAutoLockMinutes(v);
        }
    }
    if (flags.contains("device_status")) {
        const QString v = flags.value("device_status").toString();
        if (v != m_deviceStatus) {
            m_deviceStatus = v;
            emit deviceStatusChanged();
            if (v == QLatin1String("revoked")) {
                emit deviceRevoked();
            }
        }
    }
}

void ServerFlagsApplier::reset() {
    apply(QVariantMap{
        {"premium_active",             false},
        {"feature_export_plaintext",   false},
        {"feature_legacy_kdf_allowed", false},
        {"force_relock_required",      false},
        {"policy_message",             QString()},
        {"device_status",              QStringLiteral("trusted")},
    });
}
