#pragma once

#include "core/IPlatformSecurity.h"

class PlatformSecurityMacOS : public IPlatformSecurity {
public:
    std::unique_ptr<IDebuggerCheck>  makeDebuggerCheck()  override;
    std::unique_ptr<IProcessScanner> makeProcessScanner() override;
    std::unique_ptr<IIntegrityCheck> makeIntegrityCheck() override;
    bool isCodeSigningValid() const override;
};
