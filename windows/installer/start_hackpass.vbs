' HackPass silent launcher.
'
' Why VBS? When you double-click a .bat file, Windows opens a cmd window
' and keeps it open for the lifetime of whatever the .bat invokes - in
' our case, PowerShell, which waits for HackPass.exe to exit. That leaves
' a black cmd window on screen the entire session, which looks suspicious.
'
' wscript.exe (the VBS interpreter) has NO console window. This script
' launches launcher.ps1 hidden and exits immediately, so nothing is left
' on screen.

Option Explicit

Dim oShell, oFso, sDir
Set oShell = CreateObject("WScript.Shell")
Set oFso   = CreateObject("Scripting.FileSystemObject")
sDir = oFso.GetParentFolderName(WScript.ScriptFullName)

' Third arg 0 = hidden window. Fourth arg False = do not wait for exit.
oShell.Run "powershell -NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -File """ & sDir & "\launcher.ps1""", 0, False
