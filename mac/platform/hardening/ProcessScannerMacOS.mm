#include "ProcessScannerMacOS.h"

#include <QString>
#include <QStringList>

#include <libproc.h>
#include <vector>

// Hookable by renaming the binary or patching libproc.
bool ProcessScannerMacOS::suspiciousProcessRunning() const {
    static const QStringList kBad = {
        QStringLiteral("frida-server"),
        QStringLiteral("frida"),
        QStringLiteral("frida-helper"),
        QStringLiteral("frida-trace"),
        QStringLiteral("lldb"),
        QStringLiteral("dtrace"),
        QStringLiteral("debugserver"),
    };

    m_matched.clear();

    const int pidCount = proc_listallpids(nullptr, 0);
    if (pidCount <= 0) return false;

    std::vector<pid_t> pids(static_cast<size_t>(pidCount));
    const int got = proc_listallpids(pids.data(), pidCount * sizeof(pid_t));
    if (got <= 0) return false;

    char path[PROC_PIDPATHINFO_MAXSIZE];
    for (int i = 0; i < got; ++i) {
        if (proc_pidpath(pids[i], path, sizeof(path)) <= 0) continue;
        const QString full = QString::fromUtf8(path);
        const QString name = full.section(QLatin1Char('/'), -1).toLower();
        for (const QString& bad : kBad) {
            if (name == bad || name.startsWith(bad + QLatin1Char('-'))) {
                if (!m_matched.contains(name)) m_matched.append(name);
                break;
            }
        }
    }
    return !m_matched.isEmpty();
}

QStringList ProcessScannerMacOS::matchedProcessNames() const {
    return m_matched;
}
