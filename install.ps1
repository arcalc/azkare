<#
.SYNOPSIS
    Azkare (أذكاري) - terminal installer for Windows 10/11.
.DESCRIPTION
    Installs Git and MSYS2 (via winget, falling back to choco/scoop), installs the MinGW-w64
    toolchain + Qt6 inside MSYS2, builds Azkare from source, bundles the Qt runtime next to
    azkare.exe with windeployqt, and adds %LOCALAPPDATA%\Azkare\bin to the user PATH.
.EXAMPLE
    irm https://raw.githubusercontent.com/arcalc/azkare/main/install.ps1 | iex
.EXAMPLE
    .\install.ps1 -Update          # pull + rebuild (used by `azkare --update`)
    .\install.ps1 -Uninstall -Purge
.NOTES
    Not yet tested on a real Windows machine. Please report problems:
    https://github.com/arcalc/azkare/issues
#>
[CmdletBinding()]
param(
    [switch]$Update,
    [switch]$Uninstall,
    [switch]$Purge,        # with -Uninstall: also delete settings and source
    [switch]$Restart,      # with -Update: start Azkare again afterwards
    [switch]$Autostart     # enable start-at-login without asking
)
$ErrorActionPreference = 'Stop'

$Repo   = if ($env:AZKARE_REPO) { $env:AZKARE_REPO } else { 'https://github.com/arcalc/azkare.git' }
$Ref    = if ($env:AZKARE_REF)  { $env:AZKARE_REF }  else { 'main' }
$Root   = Join-Path $env:LOCALAPPDATA 'Azkare'
$SrcDir = Join-Path $Root 'src'
$BinDir = Join-Path $Root 'bin'
$Exe    = Join-Path $BinDir 'azkare.exe'

function Info($m) { Write-Host "[azkare] $m" -ForegroundColor Cyan }
function Ok($m)   { Write-Host "[ok] $m" -ForegroundColor Green }
function Warn($m) { Write-Host "[!] $m" -ForegroundColor Yellow }
function Has($c)  { [bool](Get-Command $c -ErrorAction SilentlyContinue) }
function Refresh-Path {
    $env:Path = [Environment]::GetEnvironmentVariable('Path', 'Machine') + ';' + [Environment]::GetEnvironmentVariable('Path', 'User')
}
function Ask($q) { (Read-Host "$q [y/N]") -match '^(y|yes)$' }

Write-Host "`n  أذكاري - Azkare installer (Windows)`n" -ForegroundColor Cyan

function Stop-Azkare {
    if (Test-Path $Exe) { try { & $Exe --quit | Out-Null } catch { } }
    for ($i = 0; $i -lt 20 -and (Get-Process -Name azkare -ErrorAction SilentlyContinue); $i++) { Start-Sleep -Milliseconds 500 }
    Get-Process -Name azkare -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
}

# ── Uninstall ────────────────────────────────────────────────────────────────
if ($Uninstall) {
    Info 'Uninstalling Azkare...'
    if (Test-Path $Exe) { try { & $Exe --autostart off | Out-Null } catch { } }
    Stop-Azkare
    Remove-ItemProperty -Path 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Run' -Name 'Azkare' -ErrorAction SilentlyContinue
    $userPath = ([Environment]::GetEnvironmentVariable('Path', 'User') -split ';') | Where-Object { $_ -and ($_ -ne $BinDir) }
    [Environment]::SetEnvironmentVariable('Path', ($userPath -join ';'), 'User')
    if (Test-Path $BinDir) { Remove-Item $BinDir -Recurse -Force -ErrorAction SilentlyContinue }
    if ($Purge -or (Ask 'Also delete your settings and the source/build folder?')) {
        Remove-Item $Root -Recurse -Force -ErrorAction SilentlyContinue
        Remove-Item (Join-Path $env:APPDATA 'Azkare') -Recurse -Force -ErrorAction SilentlyContinue
        Ok 'Settings and source removed'
    }
    Ok 'Azkare uninstalled'
    return
}

# ── Package-manager helper (winget -> choco -> scoop) ────────────────────────
function Install-Tool($Winget, $Choco, $Scoop) {
    if (Has 'winget') {
        winget install --id $Winget -e --silent --accept-package-agreements --accept-source-agreements | Out-Null
    } elseif (Has 'choco') {
        choco install $Choco -y --no-progress | Out-Null
    } elseif (Has 'scoop') {
        scoop install $Scoop | Out-Null
    } else {
        throw "No package manager found (winget/choco/scoop). Install $Winget manually and re-run."
    }
    Refresh-Path
}

function Find-Msys {
    foreach ($p in @('C:\msys64', (Join-Path $env:USERPROFILE 'scoop\apps\msys2\current'), 'C:\tools\msys64')) {
        if (Test-Path (Join-Path $p 'usr\bin\bash.exe')) { return $p }
    }
    return $null
}

function Invoke-Msys([string]$Command) {
    $env:MSYSTEM = 'UCRT64'
    $env:CHERE_INVOKING = '1'
    & (Join-Path $script:Msys 'usr\bin\bash.exe') -lc $Command
    if ($LASTEXITCODE -ne 0) { throw "MSYS2 command failed (exit $LASTEXITCODE): $Command" }
}

function Install-Deps {
    Info 'Checking build tools...'
    if (-not (Has 'git')) { Info 'Installing Git...'; Install-Tool 'Git.Git' 'git' 'git' }
    if (-not (Has 'git')) { throw 'git is still not on PATH. Open a new terminal and re-run.' }
    $script:Msys = Find-Msys
    if (-not $script:Msys) { Info 'Installing MSYS2...'; Install-Tool 'MSYS2.MSYS2' 'msys2' 'msys2'; $script:Msys = Find-Msys }
    if (-not $script:Msys) { throw 'MSYS2 was not found after installation.' }
    Info 'Installing the compiler and Qt6 inside MSYS2 (first run takes a while)...'
    Invoke-Msys 'pacman -Sy --noconfirm >/dev/null && pacman -S --needed --noconfirm mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-qt6-base mingw-w64-ucrt-x86_64-qt6-tools'
    Ok 'Dependencies ready'
}

function Get-Source {
    New-Item -ItemType Directory -Force -Path $Root | Out-Null
    if (Test-Path (Join-Path $SrcDir '.git')) {
        Info "Updating source ($Ref)..."
        git -C $SrcDir fetch --quiet --depth=1 origin $Ref
        git -C $SrcDir reset --hard --quiet FETCH_HEAD
    } elseif (Test-Path $SrcDir) {
        throw "$SrcDir exists but is not a git checkout. Move it away and re-run."
    } else {
        Info "Cloning Azkare ($Ref)..."
        git clone --quiet --depth=1 --branch $Ref $Repo $SrcDir
    }
    if ($LASTEXITCODE -ne 0) { throw 'git failed' }
    Ok ("Source: version " + (Get-Content (Join-Path $SrcDir 'VERSION')))
}

function Build-Azkare {
    if (-not $script:Msys) { $script:Msys = Find-Msys }
    if (-not $script:Msys) { throw 'MSYS2 not found. Run the installer without -Update first.' }
    Info 'Building (Release)...'
    $env:AZKARE_SRC = $SrcDir
    Invoke-Msys 'SRC=$(cygpath -u "$AZKARE_SRC"); bash "$SRC/scripts/fetch-fonts.sh" || echo "fonts: using system fallback"; cmake -S "$SRC" -B "$SRC/build" -G Ninja -DCMAKE_BUILD_TYPE=Release -DAZKARE_FETCH_FONTS=OFF && cmake --build "$SRC/build" -j'
    $built = Join-Path $SrcDir 'build\azkare.exe'
    if (-not (Test-Path $built)) { throw "Build finished but $built is missing." }
    Ok 'Build complete'
}

function Install-Binary {
    Stop-Azkare
    New-Item -ItemType Directory -Force -Path $BinDir | Out-Null
    Copy-Item (Join-Path $SrcDir 'build\azkare.exe') $Exe -Force
    $env:AZKARE_BIN = $BinDir
    Invoke-Msys 'cd "$(cygpath -u "$AZKARE_BIN")" && D=$(command -v windeployqt6 || command -v windeployqt-qt6) && "$D" --release --no-translations --compiler-runtime azkare.exe'
    $userPath = [Environment]::GetEnvironmentVariable('Path', 'User')
    if (($userPath -split ';') -notcontains $BinDir) {
        [Environment]::SetEnvironmentVariable('Path', ($userPath.TrimEnd(';') + ';' + $BinDir), 'User')
        $env:Path += ";$BinDir"
        Ok "Added $BinDir to your PATH"
    }
    Ok ("Installed: " + (& $Exe --version))
}

# ── Main ─────────────────────────────────────────────────────────────────────
if ($Update) {
    Get-Source; Build-Azkare; Install-Binary
    if ($Restart) { Start-Process -FilePath $Exe -ArgumentList '--foreground' -WindowStyle Hidden }
    Ok 'Updated.'
    return
}

Install-Deps
Get-Source
Build-Azkare
Install-Binary
if ($Autostart -or (Ask 'Start Azkare automatically when you log in?')) { & $Exe --autostart on }
Write-Host "`nAzkare is installed. Open a NEW terminal, then:" -ForegroundColor Green
Write-Host '  azkare              start in the background'
Write-Host '  azkare --settings   open settings'
Write-Host '  azkare --update     update'
Write-Host '  azkare --uninstall  remove'
