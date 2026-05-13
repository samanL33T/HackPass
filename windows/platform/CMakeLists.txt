# Windows .cpp/.rc sources contributed to the HackPass target.

target_sources(HackPass PRIVATE
    core/PlatformPathsWindows.h
    core/PlatformPathsWindows.cpp
    core/PlatformSecurityWindows.h
    core/PlatformSecurityWindows.cpp
    core/PlatformUiWindows.h
    core/PlatformUiWindows.cpp
    hardening/DebuggerCheckWindows.h
    hardening/DebuggerCheckWindows.cpp
    hardening/ProcessScannerWindows.h
    hardening/ProcessScannerWindows.cpp
    hardening/IntegrityCheckWindows.h
    hardening/IntegrityCheckWindows.cpp
    resources/app.rc
)

target_include_directories(HackPass PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/hardening
)

target_link_libraries(HackPass PRIVATE
    Advapi32
    Psapi
    Ntdll
    Dwmapi
)
