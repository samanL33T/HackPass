#pragma once

#include "core/IProcessScanner.h"

class ProcessScannerMacOS : public IProcessScanner {
public:
    bool        suspiciousProcessRunning() const override;
    QStringList matchedProcessNames()      const override;

private:
    mutable QStringList m_matched;
};
