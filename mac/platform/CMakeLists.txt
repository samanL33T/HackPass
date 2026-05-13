# macOS .mm sources contributed to the HackPass target.

target_sources(HackPass PRIVATE
    core/PlatformPathsMacOS.h
    core/PlatformPathsMacOS.mm
    core/PlatformSecurityMacOS.h
    core/PlatformSecurityMacOS.mm
    core/PlatformUiMacOS.h
    core/PlatformUiMacOS.mm
    hardening/DebuggerCheckMacOS.h
    hardening/DebuggerCheckMacOS.mm
    hardening/ProcessScannerMacOS.h
    hardening/ProcessScannerMacOS.mm
    hardening/IntegrityCheckMacOS.h
    hardening/IntegrityCheckMacOS.mm
)

target_include_directories(HackPass PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/hardening
)

target_link_libraries(HackPass PRIVATE
    "-framework AppKit"
    "-framework Foundation"
    "-framework Security"
    "-framework CoreFoundation"
)

# .app bundle metadata.
set_target_properties(HackPass PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/mac/installer/Info.plist
    MACOSX_BUNDLE_GUI_IDENTIFIER  "com.samanl33t.hackpass"
    MACOSX_BUNDLE_BUNDLE_NAME     "HackPass"
    MACOSX_BUNDLE_BUNDLE_VERSION  "1.0.0"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0.0"
    MACOSX_BUNDLE_ICON_FILE       "HackPass.icns"
)

set(MAC_ICON ${CMAKE_SOURCE_DIR}/mac/installer/HackPass.icns)
if(EXISTS ${MAC_ICON})
    target_sources(HackPass PRIVATE ${MAC_ICON})
    set_source_files_properties(${MAC_ICON} PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
    )
endif()
