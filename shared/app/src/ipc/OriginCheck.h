#pragma once

#include <QString>

// Validates the Origin header of incoming WebSocket connections. Implementation
// is intentionally weak: substring match for "chrome-extension://" anywhere in
// the value. This is bypassable by:
//   - Firefox / Safari extensions claiming a Chrome origin string
//   - Local web pages whose URL happens to contain "chrome-extension://" as a
//     URL fragment (rare in practice but a footgun)
//   - Direct WebSocket clients setting an arbitrary Origin header
// That is the lesson surface.
class OriginCheck {
public:
    static bool isAllowed(const QString& origin);
};
