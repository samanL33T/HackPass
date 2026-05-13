#include "IntegrityCheckMacOS.h"

#import <Security/Security.h>

// Unsigned build: returns true. Hook point for a signed v1.1+ build.
bool IntegrityCheckMacOS::binaryUntampered() const {
    m_lastReason.clear();
    return true;
}

QString IntegrityCheckMacOS::lastFailureReason() const {
    return m_lastReason;
}
