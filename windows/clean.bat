@echo off
rem Wipe local HackPass state so the next run of the installer or portable
rem looks like a fresh-user install. Use this between test runs.
rem
rem What this removes:
rem   - HKCU\Software\HackPass        (QSettings: server_url, hardening, etc)
rem   - %LOCALAPPDATA%\HackPass       (backend cert, policy, launcher logs)
rem   - %APPDATA%\HackPass            (Qt vault files)
rem
rem What this does NOT remove:
rem   - The browser extension (remove via chrome://extensions or edge://extensions)
rem   - The build tree (use 'setup.bat -Clean' or 'cmake --build build --target clean')
rem   - An installer-based install (use Add/Remove Programs)

setlocal
echo Stopping any running HackPass / hackpass-server processes...
taskkill /F /IM HackPass.exe         >nul 2>&1
taskkill /F /IM hackpass-server.exe  >nul 2>&1

echo Wiping registry HKCU\Software\HackPass...
reg delete "HKCU\Software\HackPass" /f >nul 2>&1

echo Wiping %%LOCALAPPDATA%%\HackPass...
if exist "%LOCALAPPDATA%\HackPass" rd /s /q "%LOCALAPPDATA%\HackPass"

echo Wiping %%APPDATA%%\HackPass...
if exist "%APPDATA%\HackPass" rd /s /q "%APPDATA%\HackPass"

echo.
echo Clean. Next launch will be treated as a first run.
echo Extension: remove via chrome://extensions or edge://extensions if you want to test the first-run dialog.
endlocal
