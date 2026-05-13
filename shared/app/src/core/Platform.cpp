#include "Platform.h"

#include <QtGlobal>

// Headers come from the platform-specific include dir added by either
// windows/platform/CMakeLists.txt or mac/platform/CMakeLists.txt.
#if defined(Q_OS_WIN)
    #include "PlatformPathsWindows.h"
    #include "PlatformSecurityWindows.h"
    #include "PlatformUiWindows.h"
#elif defined(Q_OS_MACOS)
    #include "PlatformPathsMacOS.h"
    #include "PlatformSecurityMacOS.h"
    #include "PlatformUiMacOS.h"
#else
    #error "Unsupported platform - only Windows (v1.0) and macOS (v1.1) are supported"
#endif

IPlatformPaths& Platform::paths() {
#if defined(Q_OS_WIN)
    static PlatformPathsWindows instance;
#elif defined(Q_OS_MACOS)
    static PlatformPathsMacOS instance;
#endif
    return instance;
}

IPlatformSecurity& Platform::security() {
#if defined(Q_OS_WIN)
    static PlatformSecurityWindows instance;
#elif defined(Q_OS_MACOS)
    static PlatformSecurityMacOS instance;
#endif
    return instance;
}

IPlatformUi& Platform::ui() {
#if defined(Q_OS_WIN)
    static PlatformUiWindows instance;
#elif defined(Q_OS_MACOS)
    static PlatformUiMacOS instance;
#endif
    return instance;
}
