#pragma once

#include "core/IDebuggerCheck.h"

class DebuggerCheckWindows : public IDebuggerCheck {
public:
    bool    isDebuggerPresent()   const override;
    QString lastDetectionReason() const override;

private:
    mutable QString m_lastReason;
};
