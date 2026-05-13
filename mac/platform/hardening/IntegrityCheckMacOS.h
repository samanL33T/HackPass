#pragma once

#include "core/IIntegrityCheck.h"

class IntegrityCheckMacOS : public IIntegrityCheck {
public:
    bool    binaryUntampered() const override;
    QString lastFailureReason() const override;

private:
    mutable QString m_lastReason;
};
