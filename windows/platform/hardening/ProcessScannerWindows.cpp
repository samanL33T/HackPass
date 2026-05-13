#include "ProcessScannerWindows.h"

#include <QStringList>

#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>

namespace {

const QStringList& suspiciousList() {
    static const QStringList list = {
        QStringLiteral("frida-server.exe"),
        QStringLiteral("frida-helper.exe"),
        QStringLiteral("frida-helper-32.exe"),
        QStringLiteral("frida-helper-64.exe"),
        QStringLiteral("fridaserver.exe"),
        QStringLiteral("frida-gadget.exe"),
        QStringLiteral("x64dbg.exe"),
        QStringLiteral("x32dbg.exe"),
        QStringLiteral("ollydbg.exe"),
        QStringLiteral("ida.exe"),
        QStringLiteral("ida64.exe"),
        QStringLiteral("idaq.exe"),
        QStringLiteral("idaq64.exe"),
        QStringLiteral("wireshark.exe"),
        QStringLiteral("dumpcap.exe"),
        QStringLiteral("processhacker.exe"),
        QStringLiteral("procmon.exe"),
        QStringLiteral("procmon64.exe"),
        QStringLiteral("cheatengine-x86_64.exe"),
    };
    return list;
}

}  // namespace

bool ProcessScannerWindows::suspiciousProcessRunning() const {
    m_matched.clear();
    HANDLE snap = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) {
        return false;
    }
    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);
    if (::Process32FirstW(snap, &entry)) {
        do {
            const QString name = QString::fromWCharArray(entry.szExeFile).toLower();
            for (const auto& susp : suspiciousList()) {
                if (name == susp.toLower()) {
                    m_matched.append(name);
                    break;
                }
            }
        } while (::Process32NextW(snap, &entry));
    }
    ::CloseHandle(snap);
    return !m_matched.isEmpty();
}

QStringList ProcessScannerWindows::matchedProcessNames() const {
    return m_matched;
}
