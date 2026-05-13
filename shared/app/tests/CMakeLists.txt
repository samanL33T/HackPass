function(hackpass_add_test name source)
    qt_add_executable(${name} ${source}
        ../src/crypto/KeyDerivation.cpp
        ../src/crypto/VaultCrypto.cpp
        ../src/crypto/LegacyXOR.cpp
        ../src/crypto/HmacLicense.cpp
        ../src/vault/VaultEntry.cpp
        ../src/vault/VaultFile.cpp
        ../src/vault/VaultModel.cpp
        ../src/vault/VaultManager.cpp
        ../src/settings/AppSettings.cpp
        ../src/settings/TokenStore.cpp
        ../src/ipc/TokenHandshake.cpp
    )
    target_include_directories(${name} PRIVATE ../src)
    target_link_libraries(${name} PRIVATE
        Qt6::Core
        Qt6::Test
        Qt6::Concurrent
        OpenSSL::Crypto
    )
    add_test(NAME ${name} COMMAND ${name})

    # On Windows, prepend Qt and OpenSSL bin dirs to PATH so the test executables
    # can locate their runtime DLLs without manual environment setup. Uses
    # ENVIRONMENT_MODIFICATION (CMake 3.22+) which merges with the inherited PATH.
    if(WIN32)
        set(_qt_bin "$<TARGET_FILE_DIR:Qt6::Core>")
        set(_ossl_bin "${OPENSSL_ROOT_DIR}/bin")
        set_tests_properties(${name} PROPERTIES
            ENVIRONMENT_MODIFICATION
                "PATH=path_list_prepend:${_qt_bin};PATH=path_list_prepend:${_ossl_bin}"
        )
    endif()
endfunction()

hackpass_add_test(test_keyderivation test_keyderivation.cpp)
hackpass_add_test(test_vaultcrypto   test_vaultcrypto.cpp)
hackpass_add_test(test_legacyxor     test_legacyxor.cpp)
hackpass_add_test(test_hmaclicense   test_hmaclicense.cpp)
hackpass_add_test(test_vaultfile     test_vaultfile.cpp)
hackpass_add_test(test_vaultmanager  test_vaultmanager.cpp)
hackpass_add_test(test_appsettings   test_appsettings.cpp)
hackpass_add_test(test_tokenhandshake test_tokenhandshake.cpp)
