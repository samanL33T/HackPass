#include "DebuggerCheckWindows.h"

#include <windows.h>
#include <winternl.h>

// Forward-declare NtQueryInformationProcess so we can resolve dynamically and
// avoid linking against ntdll.lib import directly.
typedef NTSTATUS(NTAPI* fnNtQueryInformationProcess)(
    HANDLE, ULONG, PVOID, ULONG, PULONG);

namespace {

constexpr ULONG ProcessDebugPort_ = 7;
constexpr ULONG ProcessDebugFlags_ = 31;

bool checkIsDebuggerPresent()         { return ::IsDebuggerPresent() != 0; }

bool checkRemoteDebugger() {
    BOOL flag = FALSE;
    if (!::CheckRemoteDebuggerPresent(::GetCurrentProcess(), &flag)) return false;
    return flag != FALSE;
}

bool checkProcessDebugPort() {
    static fnNtQueryInformationProcess pNtQip = reinterpret_cast<fnNtQueryInformationProcess>(
        ::GetProcAddress(::GetModuleHandleW(L"ntdll.dll"), "NtQueryInformationProcess"));
    if (!pNtQip) return false;
    DWORD_PTR debugPort = 0;
    ULONG ret = 0;
    if (pNtQip(::GetCurrentProcess(), ProcessDebugPort_, &debugPort, sizeof(debugPort), &ret) != 0) return false;
    return debugPort != 0;
}

bool checkProcessDebugFlags() {
    static fnNtQueryInformationProcess pNtQip = reinterpret_cast<fnNtQueryInformationProcess>(
        ::GetProcAddress(::GetModuleHandleW(L"ntdll.dll"), "NtQueryInformationProcess"));
    if (!pNtQip) return false;
    DWORD debugFlags = 0;
    ULONG ret = 0;
    if (pNtQip(::GetCurrentProcess(), ProcessDebugFlags_, &debugFlags, sizeof(debugFlags), &ret) != 0) return false;
    // 0 means "being debugged"; 1 means "not being debugged"
    return debugFlags == 0;
}

bool checkPebBeingDebugged() {
#ifdef _M_X64
    auto* peb = reinterpret_cast<unsigned char*>(__readgsqword(0x60));
    return peb && peb[2] != 0;
#else
    return false;
#endif
}

}  // namespace

bool DebuggerCheckWindows::isDebuggerPresent() const {
    if (checkIsDebuggerPresent())      { m_lastReason = QStringLiteral("IsDebuggerPresent");          return true; }
    if (checkRemoteDebugger())          { m_lastReason = QStringLiteral("CheckRemoteDebuggerPresent"); return true; }
    if (checkProcessDebugPort())        { m_lastReason = QStringLiteral("ProcessDebugPort");           return true; }
    if (checkProcessDebugFlags())       { m_lastReason = QStringLiteral("ProcessDebugFlags");          return true; }
    if (checkPebBeingDebugged())        { m_lastReason = QStringLiteral("PEB.BeingDebugged");          return true; }
    m_lastReason.clear();
    return false;
}

QString DebuggerCheckWindows::lastDetectionReason() const {
    return m_lastReason;
}
