# HackPass setup script.
#
# Checks prereqs, finds Qt and OpenSSL, builds the desktop app, deploys Qt DLLs,
# starts the backend with bun. Skips parts gracefully if a tool is missing.
#
#   setup.ps1                run everything
#   setup.ps1 -SkipPrereq    skip the prereq check
#   setup.ps1 -SkipApp       only do the backend
#   setup.ps1 -SkipBackend   only build the app
#   setup.ps1 -Clean         wipe the build dir first

[CmdletBinding()]
param(
    [switch]$SkipPrereq,
    [switch]$SkipApp,
    [switch]$SkipBackend,
    [switch]$Clean
)

$ErrorActionPreference = "Stop"
# This script lives at windows/setup.ps1; repo root is one level up.
$repo = Split-Path $PSScriptRoot -Parent

function Write-Step($msg)  { Write-Host "==> $msg" -ForegroundColor Cyan }
function Write-Ok($msg)    { Write-Host "    ok: $msg" -ForegroundColor Green }
function Write-Warn2($msg) { Write-Host "    warn: $msg" -ForegroundColor Yellow }
function Write-Err($msg)   { Write-Host "    error: $msg" -ForegroundColor Red }

function Test-CommandExists($name) {
    $null -ne (Get-Command $name -ErrorAction SilentlyContinue)
}

function Find-QtPath {
    if ($env:Qt6_DIR -and (Test-Path "$env:Qt6_DIR\bin\qmake.exe")) {
        return $env:Qt6_DIR
    }
    $candidates = @()
    foreach ($root in @("C:\Qt", "D:\Qt")) {
        if (Test-Path $root) {
            $candidates += Get-ChildItem $root -Directory -ErrorAction SilentlyContinue |
                Where-Object { $_.Name -match '^6\.\d+' } |
                ForEach-Object {
                    Join-Path $_.FullName "msvc2022_64"
                } |
                Where-Object { Test-Path (Join-Path $_ "bin\qmake.exe") }
        }
    }
    if ($candidates.Count -gt 0) {
        return ($candidates | Sort-Object -Descending | Select-Object -First 1)
    }
    return $null
}

function Find-OpenSslPath {
    $candidates = @(
        "C:\Program Files\FireDaemon OpenSSL 4",
        "C:\Program Files\FireDaemon OpenSSL 3",
        "C:\Program Files\OpenSSL-Win64",
        "C:\Qt\Tools\OpenSSLv3\Win_x64"
    )
    foreach ($p in $candidates) {
        if (Test-Path (Join-Path $p "include\openssl\evp.h")) {
            return $p
        }
    }
    return $null
}

# Prereqs -----------------------------------------------------------

if (-not $SkipPrereq) {
    Write-Step "Checking prerequisites"

    $allOk = $true

    if (Test-CommandExists "cmake") {
        $v = (cmake --version | Select-Object -First 1)
        Write-Ok "cmake: $v"
    } else {
        Write-Err "cmake not in PATH. Install from https://cmake.org/download/ (or 'winget install Kitware.CMake')."
        $allOk = $false
    }

    if (Test-CommandExists "cl") {
        Write-Ok "MSVC compiler (cl.exe) available"
    } else {
        Write-Warn2 "cl.exe not in PATH. Run this from a 'x64 Native Tools Command Prompt for VS 2022' or install Visual Studio with the C++ workload."
        Write-Warn2 "CMake's Visual Studio generator can still find it, so we'll try."
    }

    $qt = Find-QtPath
    if ($qt) {
        Write-Ok "Qt found at $qt"
        $env:CMAKE_PREFIX_PATH = $qt
        $env:Qt6_DIR = $qt
    } else {
        Write-Err "Qt 6.7+ for MSVC 2022 not found. Install via Qt online installer with the 'MSVC 2022 64-bit' component."
        $allOk = $false
    }

    $ossl = Find-OpenSslPath
    if ($ossl) {
        Write-Ok "OpenSSL found at $ossl"
        $env:OPENSSL_ROOT_DIR = $ossl
    } else {
        Write-Warn2 "OpenSSL dev headers not found in common locations. Install with 'winget install FireDaemon.OpenSSL' or set OPENSSL_ROOT_DIR."
        Write-Warn2 "CMake's bundled FindOpenSSL may still locate it; we'll try."
    }

    if (Test-CommandExists "bun") {
        Write-Ok "bun: $(bun --version)"
    } else {
        Write-Warn2 "bun not in PATH. Backend will not start automatically. Install via: powershell -c 'irm bun.sh/install.ps1 | iex'"
    }

    if (-not $allOk) {
        Write-Host ""
        Write-Err "Missing required tools. Install them and re-run, or pass -SkipPrereq if you know what you're doing."
        exit 1
    }
} else {
    Write-Warn2 "Prereq check skipped."
}

# App build ---------------------------------------------------------

if (-not $SkipApp) {
    Push-Location $repo
    try {
        if ($Clean -and (Test-Path "build")) {
            Write-Step "Cleaning build/"
            Remove-Item "build" -Recurse -Force
        }

        Write-Step "Configuring (cmake)"
        $configureArgs = @("-S", ".", "-B", "build")
        if ($env:CMAKE_PREFIX_PATH) {
            $configureArgs += "-DCMAKE_PREFIX_PATH=$($env:CMAKE_PREFIX_PATH)"
        }
        if ($env:OPENSSL_ROOT_DIR) {
            $configureArgs += "-DOPENSSL_ROOT_DIR=$($env:OPENSSL_ROOT_DIR)"
        }
        cmake @configureArgs
        if ($LASTEXITCODE -ne 0) { throw "cmake configure failed" }

        Write-Step "Building (cmake --build, Release)"
        cmake --build build --config Release
        if ($LASTEXITCODE -ne 0) { throw "cmake build failed" }

        # MSBuild VS generator drops the exe at build\Release\, Ninja at
        # build\app\Release\. Try both, then fall back to a recursive search.
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
            throw "Built but HackPass.exe not found under build\. cmake build may have failed silently."
        }
        Write-Ok "Built exe at $exe"

        $qt = $env:Qt6_DIR
        if ($qt -and (Test-Path (Join-Path $qt "bin\windeployqt.exe"))) {
            Write-Step "Bundling Qt DLLs (windeployqt --release --compiler-runtime)"
            # --release          force release flavour of every dep
            # --compiler-runtime copy the MSVC redistributable DLLs (vcruntime140.dll etc)
            #                    so the app launches on machines without MSVC installed.
            # --qmldir           tell windeployqt where the .qml source lives so it can
            #                    detect which Qt QML modules to bundle.
            & (Join-Path $qt "bin\windeployqt.exe") `
                --release `
                --compiler-runtime `
                --qmldir (Join-Path $repo "shared\app\src\resources\qml") `
                $exe
            if ($LASTEXITCODE -ne 0) { Write-Warn2 "windeployqt returned $LASTEXITCODE - app may still run, check missing DLLs manually." }

            # Slim the deployed Qt tree: drop software-OpenGL fallback, QML
            # debugger plugins, unused control styles, and non-English
            # translations. Saves ~30-40 MB in the final zip.
            Write-Step "Slimming deployed Qt tree"
            $exeDir = Split-Path $exe -Parent
            # Qt 6.7+ defaults to D3D11 for Qt Quick RHI. d3dcompiler_47.dll
            # is the D3D11 shader compiler (keep). dxcompiler.dll + dxil.dll
            # are the D3D12 pair - only loaded when QSG_RHI_BACKEND=d3d12.
            # We don't request D3D12, so drop both (saves ~35 MB).
            # Qt6Quick3DUtils.dll is only used by qmldbg_quick3dprofiler.dll,
            # which lives under qmltooling/ and is already dropped.
            $drop = @(
                "opengl32sw.dll",
                "vc_redist.x64.exe",
                "dxcompiler.dll",
                "dxil.dll",
                "Qt6Quick3DUtils.dll"
            )
            foreach ($f in $drop) {
                $p = Join-Path $exeDir $f
                if (Test-Path $p) { Remove-Item $p -Force; Write-Ok "  dropped $f" }
            }
            $qmltooling = Join-Path $exeDir "qmltooling"
            if (Test-Path $qmltooling) { Remove-Item $qmltooling -Recurse -Force; Write-Ok "  dropped qmltooling/" }
            $controlsDir = Join-Path $exeDir "qml\QtQuick\Controls"
            foreach ($style in @("Imagine","Fusion","Universal","FluentWinUI3","Windows")) {
                $sub = Join-Path $controlsDir $style
                if (Test-Path $sub) { Remove-Item $sub -Recurse -Force; Write-Ok "  dropped qml/QtQuick/Controls/$style/" }
            }
            $ns = Join-Path $exeDir "qml\QtQuick\NativeStyle"
            if (Test-Path $ns) { Remove-Item $ns -Recurse -Force; Write-Ok "  dropped qml/QtQuick/NativeStyle/" }

            # Image format plugins. PNG is built into Qt6Gui (no separate DLL).
            # We only use SVG (the logo) - drop everything else.
            $imgFmt = Join-Path $exeDir "imageformats"
            if (Test-Path $imgFmt) {
                foreach ($f in @("qgif.dll","qicns.dll","qico.dll","qjpeg.dll","qtga.dll","qtiff.dll","qwbmp.dll","qwebp.dll")) {
                    $p = Join-Path $imgFmt $f
                    if (Test-Path $p) { Remove-Item $p -Force; Write-Ok "  dropped imageformats/$f" }
                }
            }

            # Unused QML submodules. HackPass imports only QtQuick, Controls,
            # Controls.Material, Layouts, Window. Everything else gets dropped.
            foreach ($sub in @(
                "qml\QtQuick\Particles",
                "qml\QtQuick\Dialogs",
                "qml\QtQuick\Timeline",
                "qml\QtQuick\LocalStorage",
                "qml\QtQuick\VectorImage",
                "qml\QtQml\XmlListModel"
            )) {
                $p = Join-Path $exeDir $sub
                if (Test-Path $p) { Remove-Item $p -Recurse -Force; Write-Ok "  dropped $sub/" }
            }
            $trans = Join-Path $exeDir "translations"
            if (Test-Path $trans) {
                Get-ChildItem $trans -Filter "qt_*.qm" -ErrorAction SilentlyContinue |
                    Where-Object { $_.Name -ne "qt_en.qm" } |
                    Remove-Item -Force
                Write-Ok "  trimmed translations to qt_en.qm only"
            }
        } else {
            Write-Warn2 "windeployqt not found. The app may fail to launch outside the Qt prompt."
        }

        # OpenSSL DLLs (loose, next to HackPass.exe). Glob-match because the
        # filename varies by OpenSSL version + repackager:
        #   OpenSSL 3.x stock:    libcrypto-3-x64.dll, libssl-3-x64.dll
        #   FireDaemon "4":       sometimes renames to -4-x64, sometimes keeps -3-x64
        $exeDir = Split-Path $exe -Parent
        if ($env:OPENSSL_ROOT_DIR) {
            $opensslBin = Join-Path $env:OPENSSL_ROOT_DIR "bin"
            Write-Step "Copying OpenSSL DLLs from $opensslBin"
            $dlls = Get-ChildItem -Path $opensslBin -Filter "*.dll" -ErrorAction SilentlyContinue |
                    Where-Object { $_.Name -match '^lib(crypto|ssl)' }
            if ($dlls) {
                foreach ($f in $dlls) {
                    Copy-Item $f.FullName $exeDir -Force
                    Write-Ok "  $($f.Name)"
                }
            } else {
                Write-Warn2 "No libcrypto*/libssl* DLLs found under $opensslBin. Listing contents:"
                Get-ChildItem -Path $opensslBin -Filter "*.dll" -ErrorAction SilentlyContinue |
                    ForEach-Object { Write-Warn2 "    $($_.Name)" }
            }
        } else {
            Write-Warn2 "OPENSSL_ROOT_DIR not set - OpenSSL DLLs were not copied. App will not launch on user machines."
        }

        # MSVC runtime DLLs. windeployqt --compiler-runtime drops vc_redist.x64.exe
        # (the installer) since Qt 6.0, not the loose DLLs we need for a portable
        # bundle. Search VS's redist dir, then VCToolsRedistDir env, then System32.
        Write-Step "Copying MSVC runtime DLLs"
        $msvcDlls = @("vcruntime140.dll", "vcruntime140_1.dll", "msvcp140.dll")
        $searchDirs = @()
        if ($env:VCToolsRedistDir) {
            $candidate = Join-Path $env:VCToolsRedistDir "x64\Microsoft.VC143.CRT"
            if (Test-Path $candidate) { $searchDirs += $candidate }
        }
        # Common VS 2022 redist locations.
        foreach ($pat in @(
            "${env:ProgramFiles}\Microsoft Visual Studio\2022\*\VC\Redist\MSVC\*\x64\Microsoft.VC143.CRT",
            "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\*\VC\Redist\MSVC\*\x64\Microsoft.VC143.CRT"
        )) {
            $matches = Get-Item -Path $pat -ErrorAction SilentlyContinue
            if ($matches) { $searchDirs += ($matches | ForEach-Object FullName) }
        }
        $searchDirs += "${env:SystemRoot}\System32"

        foreach ($dll in $msvcDlls) {
            $copied = $false
            foreach ($dir in $searchDirs) {
                $src = Join-Path $dir $dll
                if (Test-Path $src) {
                    Copy-Item $src $exeDir -Force
                    Write-Ok "  $dll (from $dir)"
                    $copied = $true
                    break
                }
            }
            if (-not $copied) {
                Write-Warn2 "  $dll not found - user machines may need VC++ Redistributable"
            }
        }
    } finally {
        Pop-Location
    }
}

# Backend -----------------------------------------------------------

if (-not $SkipBackend) {
    $backend = Join-Path $repo "shared\backend"
    if (Test-CommandExists "bun") {
        Write-Step "Starting backend with bun (background process)"
        Push-Location $backend
        try {
            if (-not (Test-Path "node_modules")) { bun install }
            Start-Process -FilePath "bun" -ArgumentList "run","start" -WorkingDirectory $backend
            Write-Ok "Backend started at https://127.0.0.1:8443"
        } finally {
            Pop-Location
        }

        # Align dev mode with the dev backend port. If the user previously
        # ran the installer, the QSettings server_url in the registry points
        # at the launcher's random port - and the source-built HackPass.exe
        # would otherwise try to reach that port and silently fail.
        $regPath = "HKCU:\Software\HackPass\app"
        if (-not (Test-Path $regPath)) { New-Item -Path $regPath -Force | Out-Null }
        Set-ItemProperty -Path $regPath -Name "server_url" -Value "https://localhost:8443" -Type String -ErrorAction SilentlyContinue
        Write-Ok "Dev mode: server_url set to https://localhost:8443 (HKCU\Software\HackPass\app)"
    } else {
        Write-Warn2 "bun not on PATH - skipping backend. Install bun (powershell -c 'irm bun.sh/install.ps1 | iex') and re-run with -SkipApp."
    }
}

# Summary -----------------------------------------------------------

Write-Host ""
Write-Step "Done"
if (-not $SkipApp) {
    $exeFinal = $null
    foreach ($p in @(
        "$repo\build\Release\HackPass.exe",
        "$repo\build\app\Release\HackPass.exe"
    )) {
        if (Test-Path $p) { $exeFinal = $p; break }
    }
    if ($exeFinal) {
        Write-Host "  App:       $exeFinal"
    }
}
if (-not $SkipBackend) {
    Write-Host "  Backend:   https://localhost:8443  (config: backend\data\policy.json)"
}
Write-Host "  Extension: chrome://extensions -> Developer mode -> Load unpacked -> select $(Join-Path $repo 'shared\extension')"
Write-Host ""
Write-Host "Reminder: HackPass is intentionally vulnerable. Do not store real passwords." -ForegroundColor Yellow
