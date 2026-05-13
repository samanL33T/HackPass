# Builds the release artifacts in installer\output\:
#   HackPass-Portable-X.Y.Z.zip   (always)
#   HackPass-Setup-X.Y.Z.exe      (if Inno Setup 6 is installed)
#
# Run setup.ps1 first to produce the app build; bun must be on PATH for
# the standalone backend compile.

[CmdletBinding()]
param(
    [string]$Version = "1.0.0",
    [switch]$SkipApp,
    [switch]$SkipBackendCompile,
    [switch]$NoInstaller,
    [switch]$NoPortable
)

$ErrorActionPreference = "Stop"
# This script lives at windows/installer/package.ps1; repo root is TWO
# levels up (windows/installer -> windows -> root).
$repo      = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$installer = $PSScriptRoot
$stage     = Join-Path $installer "stage"
$output    = Join-Path $installer "output"

function Write-Step($msg) { Write-Host "==> $msg" -ForegroundColor Cyan }
function Write-Ok($msg)   { Write-Host "    ok: $msg" -ForegroundColor Green }
function Write-Warn2($msg){ Write-Host "    warn: $msg" -ForegroundColor Yellow }

# Reset stage + output dirs.
foreach ($d in @($stage, $output)) {
    if (Test-Path $d) { Remove-Item $d -Recurse -Force }
    New-Item -ItemType Directory -Path $d -Force | Out-Null
}

# 1. App build -----------------------------------------------------

if (-not $SkipApp) {
    # MSBuild VS generator → build\Release\, Ninja/Make → build\app\Release\.
    # Locate the exe first, then derive its directory.
    $exe = $null
    foreach ($p in @(
        "$repo\build\Release\HackPass.exe",
        "$repo\build\app\Release\HackPass.exe"
    )) {
        if (Test-Path $p) { $exe = $p; break }
    }
    if (-not $exe) {
        $found = Get-ChildItem -Path "$repo\build" -Filter "HackPass.exe" -Recurse -ErrorAction SilentlyContinue |
                 Where-Object { $_.FullName -match '\\Release\\' } |
                 Sort-Object LastWriteTime -Descending |
                 Select-Object -First 1
        if ($found) { $exe = $found.FullName }
    }
    if (-not $exe) {
        throw "HackPass.exe not found anywhere under build\. Run setup.ps1 first (or check that it succeeded)."
    }
    $appBuildDir = Split-Path $exe -Parent
    Write-Step "Staging app build output from $appBuildDir"
    Copy-Item -Path "$appBuildDir\*" -Destination $stage -Recurse -Force

    # Tests are built into the same directory by setup.ps1 (it's an MSBuild
    # quirk). They're not part of the release - drop them after staging.
    Get-ChildItem -Path $stage -Filter "test_*.*" -ErrorAction SilentlyContinue | Remove-Item -Recurse -Force
    Get-ChildItem -Path $stage -Filter "*.lib"    -ErrorAction SilentlyContinue | Remove-Item -Force
    Get-ChildItem -Path $stage -Filter "*.exp"    -ErrorAction SilentlyContinue | Remove-Item -Force
    Get-ChildItem -Path $stage -Filter "*.pdb"    -ErrorAction SilentlyContinue | Remove-Item -Force

    # Sanity-check the runtime deps that always trip first-launch on a clean machine.
    # Exact-name checks for things with stable names; pattern matches for OpenSSL
    # because its DLL filenames vary across versions / repackagers.
    $requiredExact = @("Qt6Core.dll", "Qt6Gui.dll", "Qt6Qml.dll", "Qt6Quick.dll", "vcruntime140.dll")
    $requiredPatterns = @{ "libcrypto" = "libcrypto*.dll"; "libssl" = "libssl*.dll" }

    $missing = @()
    foreach ($dll in $requiredExact) {
        if (-not (Test-Path (Join-Path $stage $dll))) { $missing += $dll }
    }
    foreach ($name in $requiredPatterns.Keys) {
        $found = Get-ChildItem -Path $stage -Filter $requiredPatterns[$name] -ErrorAction SilentlyContinue
        if (-not $found) { $missing += "$name*.dll" }
    }
    if ($missing.Count -gt 0) {
        Write-Warn2 "Missing runtime DLLs in stage: $($missing -join ', ')"
        Write-Warn2 "Re-run setup.ps1 - it bundles windeployqt + OpenSSL automatically."
        throw "Stage incomplete. The installer would ship a broken app."
    }
    Write-Ok "App files staged (runtime DLLs present)"
}

# 2. Backend compile ----------------------------------------------

if (-not $SkipBackendCompile) {
    $backendDir = Join-Path $repo "shared\backend"
    $bun = Get-Command "bun" -ErrorAction SilentlyContinue
    if (-not $bun) {
        Write-Warn2 "bun not found on PATH; skipping backend compile. The installer/portable will not include the backend."
    } else {
        # bun build --compile + cross-compile target landed in bun 1.1.x.
        # Reject ancient bun fast with a clear message rather than producing
        # broken output an hour later.
        $bunVer = (& bun --version).Trim()
        $major  = ($bunVer -split '\.')[0]
        $minor  = ($bunVer -split '\.')[1]
        if ([int]$major -lt 1 -or ([int]$major -eq 1 -and [int]$minor -lt 1)) {
            throw "bun $bunVer is too old. Need bun 1.1+ for cross-platform --compile. Upgrade with: powershell -c 'irm bun.sh/install.ps1 | iex'"
        }
        Write-Step "Compiling backend with bun $bunVer (--compile, target bun-windows-x64)"
        Push-Location $backendDir
        try {
            if (-not (Test-Path "node_modules")) { bun install }
            $serverOut = Join-Path $stage "hackpass-server.exe"
            # --minify        strips whitespace + shortens identifiers in the JS bundle
            # --sourcemap=none skip the embedded sourcemap (not useful in a release binary)
            # Together they shave a few MB off the JS portion. The bulk of the
            # binary is the Bun runtime itself, which we cannot slim further.
            bun build src/server.ts --compile --minify --sourcemap=none --outfile $serverOut --target=bun-windows-x64
            if ($LASTEXITCODE -ne 0 -or -not (Test-Path $serverOut)) {
                throw "bun build --compile failed (exit $LASTEXITCODE). Did not produce $serverOut"
            }
            $sizeMb = [math]::Round((Get-Item $serverOut).Length / 1MB, 2)
            Write-Ok "hackpass-server.exe staged ($sizeMb MB)"
        } finally {
            Pop-Location
        }

        # Stage openssl.exe next to the backend. tlsCert.ts prefers this
        # bundled copy so the end-user machine does not need OpenSSL on PATH.
        $opensslRoot = $env:OPENSSL_ROOT_DIR
        if (-not $opensslRoot) {
            foreach ($p in @(
                "C:\Program Files\FireDaemon OpenSSL 4",
                "C:\Program Files\FireDaemon OpenSSL 3",
                "C:\Program Files\OpenSSL-Win64"
            )) {
                if (Test-Path (Join-Path $p "bin\openssl.exe")) { $opensslRoot = $p; break }
            }
        }
        if ($opensslRoot -and (Test-Path (Join-Path $opensslRoot "bin\openssl.exe"))) {
            Copy-Item (Join-Path $opensslRoot "bin\openssl.exe") $stage -Force
            Write-Ok "openssl.exe staged (from $opensslRoot)"
        } else {
            Write-Warn2 "openssl.exe not found. Set OPENSSL_ROOT_DIR before re-running, or the installer's first-run cert generation will fail on machines without OpenSSL on PATH."
        }
    }
}

# 3. Extension folder ---------------------------------------------

Write-Step "Staging extension"
Copy-Item -Path (Join-Path $repo "shared\extension") -Destination (Join-Path $stage "extension") -Recurse -Force -Exclude @("node_modules","dist","*.zip")
Write-Ok "Extension staged"

# 4. Launcher + readme --------------------------------------------

Write-Step "Staging launcher, switchpremium, VBS entry, README, icon"
Copy-Item (Join-Path $installer "launcher.ps1")           $stage
Copy-Item (Join-Path $installer "launcher.bat")           $stage
Copy-Item (Join-Path $installer "switchpremium.bat")      $stage
Copy-Item (Join-Path $installer "start_hackpass.vbs")     $stage

# Optional Windows icon. If you've generated shared/assets/icon.ico (see
# shared/tools/generate-ico.ts), stage it so Inno can use it for the Start
# Menu shortcut and the installer .exe itself.
$icoSrc = Join-Path $repo "shared\assets\icon.ico"
if (Test-Path $icoSrc) {
    Copy-Item $icoSrc $stage -Force
    Write-Ok "icon.ico staged"
} else {
    Write-Warn2 "shared\assets\icon.ico not present - Start Menu shortcut will fall back to a generic icon. Generate via: bun shared/tools/generate-ico.ts"
}

$readme = @"
HackPass $Version

Double-click start_hackpass.vbs (or the Start Menu shortcut on installed
builds) to:
  1. Start the local backend on a random localhost port
  2. Launch HackPass.exe
  3. On first run, show how to load the Chrome extension from this folder.

To enable premium features (Generate strong password, Export vault, TOTP
display), run switchpremium.bat. It opens the backend policy file in
Notepad; set "premium_active" to true, save, then relaunch HackPass.

Backend data is stored under %LOCALAPPDATA%\HackPass\backend-data so the
install folder stays read-only.

HackPass is intentionally vulnerable. Do not store real passwords. See:
  https://github.com/samanL33T/HackPass
"@
$readme | Out-File -FilePath (Join-Path $stage "README.txt") -Encoding utf8

# 5. Portable zip --------------------------------------------------

if (-not $NoPortable) {
    Write-Step "Producing portable zip"
    # Defensive: re-create output if anything wiped it between init and now
    # (Explorer handle, AV, partial Remove-Item, etc).
    if (-not (Test-Path $output)) { New-Item -ItemType Directory -Path $output -Force | Out-Null }
    $zip = Join-Path $output "HackPass-Portable-$Version.zip"
    Compress-Archive -Path (Join-Path $stage "*") -DestinationPath $zip -Force
    Write-Ok "Portable: $zip"
}

# 6. Inno Setup installer ------------------------------------------

if (-not $NoInstaller) {
    # Find ISCC.exe. Try multiple roots with verbose logging so we can see
    # exactly which paths got searched and what was rejected. The env var
    # ProgramFiles(x86) needs the [Environment]::GetEnvironmentVariable form
    # because $env:ProgramFiles(x86) is not always parsed correctly in inline
    # array literals.
    $iscc = $null
    $searchRoots = @(
        [Environment]::GetEnvironmentVariable("ProgramFiles(x86)"),
        $env:ProgramFiles,
        (Join-Path $env:LOCALAPPDATA "Programs"),
        "C:\Program Files (x86)",
        "C:\Program Files"
    ) | Where-Object { $_ } | Sort-Object -Unique

    Write-Step "Searching for ISCC.exe"
    foreach ($base in $searchRoots) {
        Write-Host "    in $base..."
        $dirs = Get-ChildItem -Path $base -Filter "Inno Setup *" -Directory -ErrorAction SilentlyContinue
        foreach ($d in $dirs) {
            $candidate = Join-Path $d.FullName "ISCC.exe"
            if (Test-Path $candidate) {
                Write-Host "      hit: $candidate"
                $iscc = $candidate
                break
            } else {
                Write-Host "      no ISCC.exe in $($d.FullName)"
            }
        }
        if ($iscc) { break }
    }
    if (-not $iscc) {
        $onPath = Get-Command "ISCC.exe" -ErrorAction SilentlyContinue
        if ($onPath) {
            Write-Host "    on PATH: $($onPath.Source)"
            $iscc = $onPath.Source
        }
    }

    if ($iscc) {
        Write-Step "Building installer with Inno Setup"
        if (-not (Test-Path $output)) { New-Item -ItemType Directory -Path $output -Force | Out-Null }
        Push-Location $installer
        try {
            & $iscc "installer.iss" "/DAppVersion=$Version"
            if ($LASTEXITCODE -ne 0) { throw "ISCC.exe returned $LASTEXITCODE" }
            Write-Ok "Installer: $(Join-Path $output "HackPass-Setup-$Version.exe")"
        } finally {
            Pop-Location
        }
    } else {
        Write-Warn2 "Inno Setup 6 not found. Install from https://jrsoftware.org/isdl.php and re-run, or pass -NoInstaller."
        Write-Warn2 "Portable zip was still produced."
    }
}

Write-Host ""
Write-Step "Release artifacts"
Get-ChildItem $output | ForEach-Object {
    Write-Host "  $($_.FullName)  ($([math]::Round($_.Length / 1MB, 2)) MB)"
}
