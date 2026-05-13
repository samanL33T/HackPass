#pragma once

#include <QString>

class IIntegrityCheck {
public:
    virtual ~IIntegrityCheck() = default;

    virtual bool    binaryUntampered() const = 0;
    virtual QString lastFailureReason() const = 0;
};
