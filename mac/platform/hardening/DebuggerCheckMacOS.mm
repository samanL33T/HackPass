#include "DebuggerCheckMacOS.h"

#include <sys/sysctl.h>
#include <sys/types.h>
#include <unistd.h>

// P_TRACED via sysctl. Hookable by patching the sysctl return.
bool DebuggerCheckMacOS::isDebuggerPresent() const {
    int          mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, getpid() };
    struct kinfo_proc info{};
    size_t       size = sizeof(info);

    if (sysctl(mib, 4, &info, &size, nullptr, 0) != 0) {
        m_lastReason = QStringLiteral("sysctl(KERN_PROC) failed");
        return false;
    }

    const bool traced = (info.kp_proc.p_flag & P_TRACED) != 0;
    m_lastReason = traced
        ? QStringLiteral("P_TRACED set on KERN_PROC info")
        : QString();
    return traced;
}

QString DebuggerCheckMacOS::lastDetectionReason() const {
    return m_lastReason;
}
