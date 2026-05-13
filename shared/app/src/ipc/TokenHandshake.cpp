#include "TokenHandshake.h"

#include "settings/TokenStore.h"

TokenHandshake::TokenHandshake(TokenStore* store) : m_store(store) {}

bool TokenHandshake::validate(const QString& received) const {
    if (!m_store) return false;
    const QString stored = m_store->token();
    if (stored.isEmpty() || received.isEmpty()) {
        return false;
    }
    // Flaw 1: first-16 truncation.
    const int n = 16;
    if (stored.size() < n || received.size() < n) {
        return false;
    }
    // Flaw 3: non-constant-time compare via QString::operator==.
    return stored.left(n) == received.left(n);
}
