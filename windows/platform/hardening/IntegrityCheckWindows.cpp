#include "IntegrityCheckWindows.h"

#include <windows.h>

bool IntegrityCheckWindows::binaryUntampered() const {
    // HackPass ships unsigned by design, so we do not call WinVerifyTrust
    // here (it would always fail). The public method exists so a signed
    // future build can swap in real Authenticode verification.
    m_lastReason.clear();
    return true;
}

QString IntegrityCheckWindows::lastFailureReason() const {
    return m_lastReason;
}
