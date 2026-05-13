#pragma once

#include <QString>

class IDebuggerCheck {
public:
    virtual ~IDebuggerCheck() = default;

    virtual bool    isDebuggerPresent() const = 0;
    virtual QString lastDetectionReason() const = 0;
};
