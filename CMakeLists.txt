cmake_minimum_required(VERSION 3.21)
project(HackPass VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Frida-hooking discipline. Preserve clean function boundaries.
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)

if(MSVC)
    add_compile_options(/W4 /permissive- /Zi /utf-8)
    add_link_options(/DEBUG:FULL)
endif()

if(APPLE)
    enable_language(OBJCXX)
    set(CMAKE_OBJCXX_STANDARD 20)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Minimum macOS version")
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.7 REQUIRED COMPONENTS
    Core
    Gui
    Qml
    Quick
    QuickControls2
    Network
    WebSockets
    Concurrent
    Test
)

# OpenSSL 3.x. Used for AES-256-GCM, HMAC-SHA256, PBKDF2, and Argon2id (when available).
if(WIN32 AND NOT DEFINED OPENSSL_ROOT_DIR)
    foreach(_candidate
            "C:/Qt/Tools/OpenSSLv3/Win_x64"
            "C:/Qt/Tools/OpenSSL/Win_x64"
            "C:/Program Files/OpenSSL-Win64"
            "C:/Program Files/FireDaemon OpenSSL 4"
            "C:/Program Files/FireDaemon OpenSSL 3")
        if(EXISTS "${_candidate}/include/openssl/evp.h")
            set(OPENSSL_ROOT_DIR "${_candidate}")
            message(STATUS "Found OpenSSL at: ${OPENSSL_ROOT_DIR}")
            break()
        endif()
    endforeach()
endif()

if(APPLE AND NOT DEFINED OPENSSL_ROOT_DIR)
    foreach(_candidate
            "/opt/homebrew/opt/openssl@3"
            "/opt/homebrew/opt/openssl"
            "/usr/local/opt/openssl@3"
            "/usr/local/opt/openssl")
        if(EXISTS "${_candidate}/include/openssl/evp.h")
            set(OPENSSL_ROOT_DIR "${_candidate}")
            message(STATUS "Found OpenSSL at: ${OPENSSL_ROOT_DIR}")
            break()
        endif()
    endforeach()
endif()

find_package(OpenSSL REQUIRED)

qt_standard_project_setup(REQUIRES 6.7)
enable_testing()

add_subdirectory(shared/app)

if(WIN32)
    add_subdirectory(windows/platform)
elseif(APPLE)
    add_subdirectory(mac/platform)
else()
    message(FATAL_ERROR "Unsupported platform - HackPass targets Windows (v1.0) and macOS (v1.1)")
endif()
