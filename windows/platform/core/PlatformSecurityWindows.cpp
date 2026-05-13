#include "PlatformSecurityWindows.h"

#include "DebuggerCheckWindows.h"
#include "ProcessScannerWindows.h"
#include "IntegrityCheckWindows.h"

std::unique_ptr<IDebuggerCheck> PlatformSecurityWindows::makeDebuggerCheck() {
    return std::make_unique<DebuggerCheckWindows>();
}

std::unique_ptr<IProcessScanner> PlatformSecurityWindows::makeProcessScanner() {
    return std::make_unique<ProcessScannerWindows>();
}

std::unique_ptr<IIntegrityCheck> PlatformSecurityWindows::makeIntegrityCheck() {
    return std::make_unique<IntegrityCheckWindows>();
}

bool PlatformSecurityWindows::isCodeSigningValid() const {
    // HackPass ships unsigned by design. See IntegrityCheckWindows.
    return false;
}
