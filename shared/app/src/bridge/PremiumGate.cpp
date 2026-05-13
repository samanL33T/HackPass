#include "PremiumGate.h"

PremiumGate::PremiumGate(QObject* parent) : QObject(parent) {}

bool PremiumGate::isPremium() const {
    return m_premium;
}

void PremiumGate::setPremium(bool v) {
    if (v == m_premium) return;
    m_premium = v;
    emit premiumChanged();
}
