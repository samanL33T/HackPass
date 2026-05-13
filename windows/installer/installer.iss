; Inno Setup script for HackPass. Compiled by installer\package.ps1 after
; the stage is built. Run manually with: ISCC.exe installer\installer.iss

#define AppName "HackPass"
#define AppVersion "1.0.0"
#define AppPublisher "samanl33t"
#define AppURL "https://samanl33t.com"

[Setup]
AppId={{B43F7AC1-B5B2-4EA8-A0A4-21F7D86D3A21}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher={#AppPublisher}
AppPublisherURL={#AppURL}
DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
; LICENSE lives at the repo root, two levels up from this .iss file.
LicenseFile=..\..\LICENSE
OutputDir=output
OutputBaseFilename={#AppName}-Setup-{#AppVersion}
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
; Use the staged .ico for the installer + uninstaller icons. If icon.ico is
; not staged (shared/assets/icon.ico missing), Inno falls back to defaults.
UninstallDisplayIcon={app}\icon.ico
SetupIconFile=stage\icon.ico

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon";   Description: "Create a desktop shortcut";       GroupDescription: "Additional shortcuts:";   Flags: unchecked
Name: "startupicon";   Description: "Launch HackPass when Windows starts"; GroupDescription: "Startup:";            Flags: unchecked

[Registry]
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "HackPass"; ValueData: """wscript.exe"" ""{app}\start_hackpass.vbs"""; Tasks: startupicon; Flags: uninsdeletevalue

[Files]
; Whole staged tree, recursively. package.ps1 lays everything out in stage\.
Source: "stage\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
; Start Menu + Desktop shortcuts target the .vbs so launching is silent
; (no cmd flash). IconFilename pulls from the staged icon.ico, falling back
; to HackPass.exe's embedded icon if the .ico is missing.
Name: "{group}\HackPass";              Filename: "{app}\start_hackpass.vbs"; IconFilename: "{app}\icon.ico"; Comment: "Launch HackPass (starts backend, opens app)"
Name: "{group}\Switch Premium";        Filename: "{app}\switchpremium.bat";  IconFilename: "{app}\icon.ico"; Comment: "Toggle premium and other server-driven flags in the backend policy file"
Name: "{group}\HackPass README";       Filename: "{app}\README.txt"
Name: "{group}\Uninstall HackPass";    Filename: "{uninstallexe}"
Name: "{commondesktop}\HackPass"; Filename: "{app}\start_hackpass.vbs"; IconFilename: "{app}\icon.ico"; Tasks: desktopicon

[Run]
Filename: "wscript.exe"; Parameters: """{app}\start_hackpass.vbs"""; Description: "Launch HackPass now"; Flags: postinstall nowait skipifsilent

[UninstallDelete]
; Only delete launcher's own state. Do NOT delete the user's vault data,
; which lives in {localappdata}\HackPass\backend-data\. A real password
; manager never silently wipes user secrets on uninstall.
Type: files; Name: "{localappdata}\HackPass\extension-instructions-shown.flag"
Type: files; Name: "{localappdata}\HackPass\launcher.log"
Type: files; Name: "{localappdata}\HackPass\backend.out.log"
Type: files; Name: "{localappdata}\HackPass\backend.err.log"
