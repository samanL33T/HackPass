#pragma once

#include <memory>

class IDebuggerCheck;
class IProcessScanner;
class IIntegrityCheck;

class IPlatformSecurity {
public:
    virtual ~IPlatformSecurity() = default;

    virtual std::unique_ptr<IDebuggerCheck>  makeDebuggerCheck()  = 0;
    virtual std::unique_ptr<IProcessScanner> makeProcessScanner() = 0;
    virtual std::unique_ptr<IIntegrityCheck> makeIntegrityCheck() = 0;
    virtual bool isCodeSigningValid() const = 0;
};
