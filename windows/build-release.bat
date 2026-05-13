@echo off
rem Build the app, compile the backend, produce installer + portable zip.
rem Result lands in installer\output\.
rem
rem Prereqs: setup.bat's prereqs (Qt 6.7+, MSVC v143, CMake 3.21+, OpenSSL)
rem          + bun 1.1+ (for the standalone backend binary)
rem          + Inno Setup 6 (optional - portable zip is always produced)

rem Build the app only. -SkipBackend prevents setup.ps1 from spawning the
rem dev backend (background bun) which we do not want when producing release
rem artifacts. package.ps1 still bun-compiles the backend into hackpass-server.exe
rem for the bundle - that is a build-time invocation, not a running service.
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0setup.ps1" -SkipBackend
if errorlevel 1 (
    echo.
    echo setup.ps1 failed. Fix the prereq it reported and re-run build-release.bat.
    exit /b 1
)

powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0installer\package.ps1"
if errorlevel 1 (
    echo.
    echo package.ps1 failed. See output above.
    exit /b 2
)

echo.
echo === Release artifacts ===
dir "%~dp0installer\output"
echo.
echo Upload the .exe and .zip from installer\output\ to GitHub Releases.
