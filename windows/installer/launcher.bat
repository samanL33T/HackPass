@echo off
rem Fallback launcher for environments where .vbs is blocked. The primary
rem entry point is start_hackpass.vbs; this .bat does the same thing but
rem leaves a cmd window open for the session.
powershell -NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -File "%~dp0launcher.ps1" %*
