@echo off
rem Double-click entry point. Runs setup.ps1 in the same directory with
rem PowerShell's execution policy temporarily relaxed for this process only.
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0setup.ps1" %*
