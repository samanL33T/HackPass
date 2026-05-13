#include "PlatformSecurityMacOS.h"

#include "DebuggerCheckMacOS.h"
#include "ProcessScannerMacOS.h"
#include "IntegrityCheckMacOS.h"

#import <Security/Security.h>
#import <Foundation/Foundation.h>

std::unique_ptr<IDebuggerCheck> PlatformSecurityMacOS::makeDebuggerCheck() {
    return std::make_unique<DebuggerCheckMacOS>();
}

std::unique_ptr<IProcessScanner> PlatformSecurityMacOS::makeProcessScanner() {
    return std::make_unique<ProcessScannerMacOS>();
}

std::unique_ptr<IIntegrityCheck> PlatformSecurityMacOS::makeIntegrityCheck() {
    return std::make_unique<IntegrityCheckMacOS>();
}

bool PlatformSecurityMacOS::isCodeSigningValid() const {
    // Unsigned by design - returns false. Hook point for a signed v1.1+ build.
    SecCodeRef code = nullptr;
    if (SecCodeCopySelf(kSecCSDefaultFlags, &code) != errSecSuccess || !code) {
        return false;
    }
    const OSStatus status = SecCodeCheckValidity(code, kSecCSDefaultFlags, nullptr);
    CFRelease(code);
    return status == errSecSuccess;
}
