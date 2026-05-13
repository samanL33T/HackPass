@echo off
rem Toggle HackPass premium (and other server-driven flags) by editing
rem the backend's policy.json in Notepad.
rem
rem Flags you can flip:
rem   "premium_active":           true to unlock premium features
rem                               (Generate strong password, Export vault, TOTP display)
rem   "feature_export_plaintext": true to allow plaintext vault export
rem   "force_relock_required":    true to force the master-password prompt
rem   "policy_message":           a string to show in the modal dialog
rem   "auto_lock_minutes":        idle-lock timeout
rem   "device_status":            "trusted" or "revoked"
rem
rem After editing: close HackPass if it's running, then relaunch via
rem start_hackpass.vbs (portable) or the Start Menu shortcut (installer).
rem The backend reads policy.json on startup, so changes take effect on
rem the next launch.

setlocal
set "DATA_DIR=%LOCALAPPDATA%\HackPass\backend-data"
set "POLICY=%DATA_DIR%\policy.json"

if not exist "%DATA_DIR%" mkdir "%DATA_DIR%" >nul 2>&1

if not exist "%POLICY%" (
    rem Seed defaults so the user has a working template even on first run.
    > "%POLICY%" echo {
    >> "%POLICY%" echo   "premium_active":             false,
    >> "%POLICY%" echo   "feature_export_plaintext":   false,
    >> "%POLICY%" echo   "feature_legacy_kdf_allowed": false,
    >> "%POLICY%" echo   "force_relock_required":      false,
    >> "%POLICY%" echo   "policy_message":             null,
    >> "%POLICY%" echo   "auto_lock_minutes":          5,
    >> "%POLICY%" echo   "device_status":              "trusted"
    >> "%POLICY%" echo }
)

notepad "%POLICY%"
endlocal
