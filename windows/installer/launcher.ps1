# HackPass launcher. Invoked by start_hackpass.vbs -> wscript -> hidden
# powershell so the user never sees a cmd window.
#
# First run picks a free localhost port and writes it to
# HKCU\Software\HackPass\app\server_url (the QSettings value the app reads).
# Subsequent runs read that value back. If the URL is non-localhost the user
# is pointing at their own backend; we don't touch it.
#
# After resolving the URL: start hackpass-server.exe hidden, wait for the
# port, launch HackPass.exe, stop the backend when the app exits.

$ErrorActionPreference = "Stop"

Add-Type -AssemblyName System.Windows.Forms | Out-Null
Add-Type -AssemblyName System.Drawing       | Out-Null

# Splash: show a small "Starting HackPass..." form immediately so the user
# has feedback during the 10s the bundled backend takes to warm up. The
# form is non-modal; we close it before launching the app proper.
$splash = New-Object System.Windows.Forms.Form
$splash.Text            = "HackPass"
$splash.FormBorderStyle = "FixedDialog"
$splash.ControlBox      = $false
$splash.StartPosition   = "CenterScreen"
$splash.Size            = New-Object System.Drawing.Size(360, 110)
$splash.BackColor       = [System.Drawing.Color]::FromArgb(20, 20, 26)
$splash.ForeColor       = [System.Drawing.Color]::FromArgb(228, 228, 232)
$splash.TopMost         = $true
$splashLabel = New-Object System.Windows.Forms.Label
$splashLabel.Text     = "Starting HackPass..."
$splashLabel.Font     = New-Object System.Drawing.Font("Segoe UI Variable", 11, [System.Drawing.FontStyle]::Regular)
$splashLabel.AutoSize = $false
$splashLabel.TextAlign = "MiddleCenter"
$splashLabel.Dock     = "Fill"
$splash.Controls.Add($splashLabel)
$splash.Show()
$splash.Refresh()

function Close-Splash {
    if ($splash -and -not $splash.IsDisposed) {
        $splash.Close()
        $splash.Dispose()
    }
}

# Paths -------------------------------------------------------------

$root         = $PSScriptRoot
$appExe       = Join-Path $root "HackPass.exe"
$serverExe    = Join-Path $root "hackpass-server.exe"
$extensionDir = Join-Path $root "extension"

$userDir   = Join-Path $env:LOCALAPPDATA "HackPass"
$dataDir   = Join-Path $userDir "backend-data"
$logFile   = Join-Path $userDir "launcher.log"
$flagFile  = Join-Path $userDir "extension-instructions-shown.flag"

if (-not (Test-Path $userDir)) { New-Item -ItemType Directory -Path $userDir -Force | Out-Null }
if (-not (Test-Path $dataDir)) { New-Item -ItemType Directory -Path $dataDir -Force | Out-Null }

function Write-Log($msg) {
    $ts = (Get-Date).ToString("yyyy-MM-dd HH:mm:ss")
    "$ts $msg" | Out-File -FilePath $logFile -Append -Encoding utf8
}

function Show-Error($title, $msg) {
    Write-Log "ERROR: $msg"
    Close-Splash
    [System.Windows.Forms.MessageBox]::Show($msg, $title, "OK", "Error") | Out-Null
}

# Get an OS-assigned free loopback port. Bind to port 0, read what the OS
# handed us, release it, return the number. Brief race window between
# release and the backend rebinding, but ephemeral ports are not
# typically reclaimed within milliseconds.
function Get-FreePort {
    $listener = New-Object System.Net.Sockets.TcpListener([System.Net.IPAddress]::Loopback, 0)
    $listener.Start()
    try { return $listener.LocalEndpoint.Port }
    finally { $listener.Stop() }
}

function Test-PortOpen($port) {
    $client = New-Object System.Net.Sockets.TcpClient
    try {
        $iar = $client.BeginConnect("127.0.0.1", $port, $null, $null)
        $ok  = $iar.AsyncWaitHandle.WaitOne(500, $false)
        if ($ok -and $client.Connected) { return $true }
        return $false
    } catch { return $false }
    finally { $client.Close() }
}

Write-Log "Launcher start. Root=$root"

# Sanity: HackPass.exe must exist.
if (-not (Test-Path $appExe)) {
    Show-Error "HackPass" "HackPass.exe was not found in:`n  $root`n`nThe install is incomplete. Please reinstall."
    exit 1
}

# Single-instance: if HackPass.exe is already running, bring its window to
# the foreground and exit. Real password managers do this so double-clicking
# the shortcut doesn't open a second copy that fights over the vault file lock.
$existing = Get-Process -Name "HackPass" -ErrorAction SilentlyContinue
if ($existing) {
    Write-Log "HackPass.exe already running (pid=$(($existing | Select-Object -First 1).Id)). Focusing existing instance."
    try {
        if (-not ("HackPassWin32" -as [type])) {
            Add-Type -Name "HackPassWin32" -Namespace "" -MemberDefinition @'
                [System.Runtime.InteropServices.DllImport("user32.dll")]
                public static extern bool SetForegroundWindow(System.IntPtr hWnd);
                [System.Runtime.InteropServices.DllImport("user32.dll")]
                public static extern bool ShowWindow(System.IntPtr hWnd, int nCmdShow);
'@
        }
        foreach ($p in $existing) {
            if ($p.MainWindowHandle -ne [System.IntPtr]::Zero) {
                [HackPassWin32]::ShowWindow($p.MainWindowHandle, 9) | Out-Null  # SW_RESTORE
                [HackPassWin32]::SetForegroundWindow($p.MainWindowHandle) | Out-Null
            }
        }
    } catch {
        Write-Log "Focus-window call failed: $($_.Exception.Message)"
    }
    Close-Splash
    exit 0
}

# Resolve backend URL ----------------------------------------------
#
# The app's QSettings backing store lives at HKCU\Software\HackPass\app.
# The 'server_url' value is the source of truth for where HackPass.exe
# tries to reach its backend. If it doesn't exist yet (first launch on
# this machine), pick a random localhost port and write it.

$regPath = "HKCU:\Software\HackPass\app"
if (-not (Test-Path $regPath)) { New-Item -Path $regPath -Force | Out-Null }

$existingUrl = $null
try {
    $prop = Get-ItemProperty -Path $regPath -Name "server_url" -ErrorAction SilentlyContinue
    if ($prop -and $prop.server_url) { $existingUrl = $prop.server_url }
} catch { }

$port          = 0
$srvHost       = "127.0.0.1"
$manageBackend = $true   # do we start hackpass-server.exe ourselves
$serverUrl     = $null

if ($existingUrl) {
    try {
        $uri       = [System.Uri]$existingUrl
        $srvHost   = $uri.Host
        $port      = $uri.Port
        $serverUrl = $existingUrl
        $isLocal   = $srvHost -in @("127.0.0.1", "localhost", "::1")
        if (-not $isLocal) {
            Write-Log "server_url points to remote host '$srvHost' - skipping local backend start."
            $manageBackend = $false
        } else {
            Write-Log "Using existing server_url=$existingUrl (port $port)"
        }
    } catch {
        Write-Log "Could not parse existing server_url '$existingUrl' - falling back to fresh assignment."
        $existingUrl = $null
    }
}

if (-not $existingUrl) {
    $port      = Get-FreePort
    $serverUrl = "https://127.0.0.1:$port"
    try {
        Set-ItemProperty -Path $regPath -Name "server_url" -Value $serverUrl -Type String
        Write-Log "First run: picked free port $port; wrote server_url=$serverUrl"
    } catch {
        Show-Error "HackPass" "Failed to write the server URL to the registry:`n$($_.Exception.Message)`n`nThe app will not be able to find the backend. Aborting."
        exit 5
    }
}

# Backend env -------------------------------------------------------

$env:HACKPASS_DATA_DIR = $dataDir
$env:HACKPASS_PORT     = "$port"

# Backend startup ---------------------------------------------------

$serverProcess   = $null
$reusingExisting = $false

if (-not $manageBackend) {
    Write-Log "Backend management skipped (remote URL configured)."
} elseif (Test-PortOpen $port) {
    Write-Log "Port $port is already open - reusing existing backend."
    $reusingExisting = $true
} elseif (Test-Path $serverExe) {
    Write-Log "Starting hackpass-server.exe on port $port"
    try {
        $serverProcess = Start-Process -FilePath $serverExe `
            -WorkingDirectory $root `
            -WindowStyle Hidden `
            -PassThru `
            -RedirectStandardOutput (Join-Path $userDir "backend.out.log") `
            -RedirectStandardError  (Join-Path $userDir "backend.err.log")
    } catch {
        Show-Error "HackPass" "Failed to start backend:`n$($_.Exception.Message)`n`nSee $logFile for details."
        exit 2
    }

    $deadline = (Get-Date).AddSeconds(15)
    $up = $false
    while ((Get-Date) -lt $deadline) {
        if ($serverProcess.HasExited) {
            $code = $serverProcess.ExitCode
            $errLog = Join-Path $userDir "backend.err.log"
            $tail = if (Test-Path $errLog) { (Get-Content $errLog -Tail 5 -ErrorAction SilentlyContinue) -join "`n" } else { "" }
            Show-Error "HackPass backend failed" "The backend process exited with code $code before becoming ready on port $port.`n`nLast log lines:`n$tail`n`nFull log: $errLog"
            exit 3
        }
        if (Test-PortOpen $port) { $up = $true; break }
        Start-Sleep -Milliseconds 250
    }
    if (-not $up) {
        Write-Log "Backend did not bind $port within 15s. Continuing anyway - app will surface the connection error."
    } else {
        Write-Log "Backend up on $port (pid=$($serverProcess.Id))."
    }
} else {
    Write-Log "No hackpass-server.exe staged - assuming user-managed backend at $serverUrl."
}

# Extension first-run instructions ----------------------------------

if ((Test-Path $extensionDir) -and (-not (Test-Path $flagFile))) {
    $msg = @"
HackPass is installed.

To enable browser autofill, load the extension in Chrome or Edge:

  1. Open the browser and go to:
       chrome://extensions   (Chrome)
       edge://extensions     (Edge)
  2. Toggle 'Developer mode' on (top right).
  3. Click 'Load unpacked' and select this folder:

       $extensionDir

  4. After unlocking your vault for the first time, copy the
     extension token from the HackPass app's Settings page and
     paste it into the extension's Options page.

The backend is running on:  $serverUrl
(You can change this in the HackPass app's Settings page.)

Reminder: HackPass is intentionally vulnerable. Do not store real passwords.
"@
    Close-Splash
    [System.Windows.Forms.MessageBox]::Show($msg, "HackPass - one-time setup", "OK", "Information") | Out-Null
    New-Item -ItemType File -Path $flagFile -Force | Out-Null
    Write-Log "Showed first-run extension instructions."
}

# Close the splash before showing the app so they don't overlap visually.
Close-Splash

# Launch the app and wait for it to exit. Clean up only the backend we started.
try {
    Write-Log "Launching HackPass.exe (backend at $serverUrl)"
    $app = Start-Process -FilePath $appExe -WorkingDirectory $root -PassThru
    $app.WaitForExit()
    Write-Log "App exited with code $($app.ExitCode)."
} catch {
    Show-Error "HackPass" "Failed to launch HackPass:`n$($_.Exception.Message)"
    exit 4
} finally {
    if ($serverProcess -and -not $reusingExisting) {
        try {
            if (-not $serverProcess.HasExited) {
                Write-Log "Stopping our backend (pid=$($serverProcess.Id))."
                Stop-Process -Id $serverProcess.Id -Force -ErrorAction SilentlyContinue
            }
        } catch {
            Write-Log "Backend cleanup error: $($_.Exception.Message)"
        }
    } elseif ($reusingExisting) {
        Write-Log "Left existing backend running (we did not start it)."
    }
}
