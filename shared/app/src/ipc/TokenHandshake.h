#pragma once

#include <QString>

class TokenStore;

// Validates the handshake token a Chrome extension presents on connect. Three
// layered weaknesses are intentional teaching surfaces:
//
//   1. Truncated compare: only the first 16 hex chars are compared.
//   2. Plaintext registry storage on the HackPass side (see TokenStore).
//   3. Non-constant-time QString::operator== for the compare itself.
class TokenHandshake {
public:
    explicit TokenHandshake(TokenStore* store);

    // Returns true if the received token matches the stored token under the
    // (deliberately weak) comparison rules.
    bool validate(const QString& receivedToken) const;

private:
    TokenStore* m_store = nullptr;
};
