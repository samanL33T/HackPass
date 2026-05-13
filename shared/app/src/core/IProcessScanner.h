#pragma once

#include <QString>
#include <QStringList>

class IProcessScanner {
public:
    virtual ~IProcessScanner() = default;

    virtual bool        suspiciousProcessRunning() const = 0;
    virtual QStringList matchedProcessNames() const = 0;
};
