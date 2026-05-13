qt_add_executable(HackPass WIN32 MACOSX_BUNDLE
    src/main.cpp
    src/core/Platform.h
    src/core/Platform.cpp
    src/core/IPlatformPaths.h
    src/core/IPlatformSecurity.h
    src/core/IPlatformUi.h
    src/core/IDebuggerCheck.h
    src/core/IProcessScanner.h
    src/core/IIntegrityCheck.h
    src/core/Logging.h
    src/core/Logging.cpp
    src/core/Clipboard.h
    src/core/Clipboard.cpp
    src/crypto/KeyDerivation.h
    src/crypto/KeyDerivation.cpp
    src/crypto/VaultCrypto.h
    src/crypto/VaultCrypto.cpp
    src/crypto/LegacyXOR.h
    src/crypto/LegacyXOR.cpp
    src/crypto/HmacLicense.h
    src/crypto/HmacLicense.cpp
    src/crypto/Totp.h
    src/crypto/Totp.cpp
    src/crypto/PasswordGenerator.h
    src/crypto/PasswordGenerator.cpp
    src/vault/VaultEntry.h
    src/vault/VaultEntry.cpp
    src/vault/VaultFile.h
    src/vault/VaultFile.cpp
    src/vault/VaultModel.h
    src/vault/VaultModel.cpp
    src/vault/VaultManager.h
    src/vault/VaultManager.cpp
    src/settings/AppSettings.h
    src/settings/AppSettings.cpp
    src/settings/TokenStore.h
    src/settings/TokenStore.cpp
    src/settings/EmbeddedLicense.h
    src/settings/EmbeddedLicense.cpp
    src/bridge/LicenseGate.h
    src/bridge/LicenseGate.cpp
    src/bridge/PremiumGate.h
    src/bridge/PremiumGate.cpp
    src/bridge/ServerFlagsApplier.h
    src/bridge/ServerFlagsApplier.cpp
    src/bridge/SettingsBridge.h
    src/bridge/SettingsBridge.cpp
    src/hardening/HardeningManager.h
    src/hardening/HardeningManager.cpp
    src/network/TofuStore.h
    src/network/TofuStore.cpp
    src/network/PinnedNetworkAccessManager.h
    src/network/PinnedNetworkAccessManager.cpp
    src/network/SyncClient.h
    src/network/SyncClient.cpp
    src/network/LicenseClient.h
    src/network/LicenseClient.cpp
    src/network/PolicyClient.h
    src/network/PolicyClient.cpp
    src/network/MessageClient.h
    src/network/MessageClient.cpp
    src/ipc/OriginCheck.h
    src/ipc/OriginCheck.cpp
    src/ipc/TokenHandshake.h
    src/ipc/TokenHandshake.cpp
    src/ipc/BrowserProtocol.h
    src/ipc/BrowserProtocol.cpp
    src/ipc/BrowserBridge.h
    src/ipc/BrowserBridge.cpp
)

# Per-platform sources + libraries are added by the platform subdirs
# (windows/platform or mac/platform) included from the root CMakeLists.txt.

# QML module: alias source files to clean module-root paths so QML can reference
# component types directly and "images/logo.svg" resolves relatively.
set_source_files_properties(src/resources/qml/Main.qml PROPERTIES
    QT_RESOURCE_ALIAS Main.qml
)
set_source_files_properties(src/resources/qml/Theme.qml PROPERTIES
    QT_RESOURCE_ALIAS Theme.qml
    QT_QML_SINGLETON_TYPE TRUE
)
set_source_files_properties(src/resources/qml/SplashScreen.qml       PROPERTIES QT_RESOURCE_ALIAS SplashScreen.qml)
set_source_files_properties(src/resources/qml/LoginPage.qml          PROPERTIES QT_RESOURCE_ALIAS LoginPage.qml)
set_source_files_properties(src/resources/qml/VaultPage.qml          PROPERTIES QT_RESOURCE_ALIAS VaultPage.qml)
set_source_files_properties(src/resources/qml/EntryDetailPage.qml    PROPERTIES QT_RESOURCE_ALIAS EntryDetailPage.qml)
set_source_files_properties(src/resources/qml/EntryEditPage.qml      PROPERTIES QT_RESOURCE_ALIAS EntryEditPage.qml)
set_source_files_properties(src/resources/qml/SettingsPage.qml       PROPERTIES QT_RESOURCE_ALIAS SettingsPage.qml)
set_source_files_properties(src/resources/qml/FirstRunWizard.qml     PROPERTIES QT_RESOURCE_ALIAS FirstRunWizard.qml)
set_source_files_properties(src/resources/qml/EntryCard.qml          PROPERTIES QT_RESOURCE_ALIAS EntryCard.qml)
set_source_files_properties(src/resources/qml/SearchBar.qml          PROPERTIES QT_RESOURCE_ALIAS SearchBar.qml)
set_source_files_properties(src/resources/qml/PasswordField.qml      PROPERTIES QT_RESOURCE_ALIAS PasswordField.qml)
set_source_files_properties(src/resources/qml/ToggleRow.qml          PROPERTIES QT_RESOURCE_ALIAS ToggleRow.qml)
set_source_files_properties(src/resources/qml/PremiumToggle.qml      PROPERTIES QT_RESOURCE_ALIAS PremiumToggle.qml)
set_source_files_properties(src/resources/qml/HardeningToggle.qml    PROPERTIES QT_RESOURCE_ALIAS HardeningToggle.qml)
set_source_files_properties(src/resources/qml/ServerMessageDialog.qml PROPERTIES QT_RESOURCE_ALIAS ServerMessageDialog.qml)
set_source_files_properties(src/resources/qml/ConfirmDialog.qml      PROPERTIES QT_RESOURCE_ALIAS ConfirmDialog.qml)
set_source_files_properties(src/resources/qml/TamperDetectedDialog.qml PROPERTIES QT_RESOURCE_ALIAS TamperDetectedDialog.qml)
set_source_files_properties(src/resources/images/logo.svg PROPERTIES
    QT_RESOURCE_ALIAS images/logo.svg
)

qt_add_qml_module(HackPass
    URI HackPass.Ui
    VERSION 1.0
    QML_FILES
        src/resources/qml/Main.qml
        src/resources/qml/Theme.qml
        src/resources/qml/SplashScreen.qml
        src/resources/qml/LoginPage.qml
        src/resources/qml/VaultPage.qml
        src/resources/qml/EntryDetailPage.qml
        src/resources/qml/EntryEditPage.qml
        src/resources/qml/SettingsPage.qml
        src/resources/qml/FirstRunWizard.qml
        src/resources/qml/EntryCard.qml
        src/resources/qml/SearchBar.qml
        src/resources/qml/PasswordField.qml
        src/resources/qml/ToggleRow.qml
        src/resources/qml/PremiumToggle.qml
        src/resources/qml/HardeningToggle.qml
        src/resources/qml/ServerMessageDialog.qml
        src/resources/qml/ConfirmDialog.qml
        src/resources/qml/TamperDetectedDialog.qml
    RESOURCES
        src/resources/images/logo.svg
)

target_include_directories(HackPass PRIVATE src)

target_link_libraries(HackPass PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Network
    Qt6::WebSockets
    Qt6::Concurrent
    OpenSSL::Crypto
)

# Platform-specific link libraries live in windows/platform/CMakeLists.txt
# and mac/platform/CMakeLists.txt.

set_target_properties(HackPass PROPERTIES
    OUTPUT_NAME HackPass
)

add_subdirectory(tests)
