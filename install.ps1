<#
.SYNOPSIS
    Azkare (أذكاري) — Zero-Touch Windows Installer
.DESCRIPTION
    Auto-installs all build dependencies (via winget/choco/scoop),
    clones Azkare, compiles with CMake, and installs the binary.
.PARAMETER Update
    Pull latest source and rebuild.
.PARAMETER Uninstall
    Remove Azkare, PATH entry, autostart, and optionally config/source.
.EXAMPLE
    irm https://raw.githubusercontent.com/arcalc/azkare/main/install.ps1 | iex
    .\install.ps1 -Update
    .\install.ps1 -Uninstall
#>
param(
    [switch]$Update,
    [switch]$Uninstall
)
$ErrorActionPreference = "Stop"

# ── Paths ──────────────────────────────────────────────────────────────────────
$Root    = "$env:LOCALAPPDATA\Azkare"
$SrcDir  = "$Root\src"
$BinDir  = "$Root\bin"
$Exe     = "$BinDir\azkare.exe"
$Repo    = "https://github.com/arcalc/azkare.git"

function Write-I  { param($m) Write-Host "[أذكاري] $m" -ForegroundColor Cyan }
function Write-Ok { param($m) Write-Host "[✓] $m"      -ForegroundColor Green }
function Write-W  { param($m) Write-Host "[!] $m"      -ForegroundColor Yellow }
function Write-E  { param($m) Write-Host "[✗] $m"      -ForegroundColor Red; exit 1 }
function Has      { param($c) return [bool](Get-Command $c -ErrorAction SilentlyContinue) }

function Refresh-Path {
    $env:PATH = [System.Environment]::GetEnvironmentVariable("PATH","Machine") + ";" +
                [System.Environment]::GetEnvironmentVariable("PATH","User")
}

function Add-UserPath { param($Dir)
    $cur = [Environment]::GetEnvironmentVariable("PATH","User")
    if ($cur -notlike "*$Dir*") {
        [Environment]::SetEnvironmentVariable("PATH","$cur;$Dir","User")
        $env:PATH += ";$Dir"
        Write-Ok "Added $Dir to PATH"
    }
}

# ── Banner ─────────────────────────────────────────────────────────────────────
Write-Host @"
  ╔══════════════════════════════════════╗
  ║   أذكاري — Azkare  Windows Install  ║
  ║   arcalc.github.io/azkare           ║
  ╚══════════════════════════════════════╝
"@ -ForegroundColor Cyan

# ── Uninstall ──────────────────────────────────────────────────────────────────
if ($Uninstall) {
    Write-I "Uninstalling Azkare..."
    if (Test-Path $Exe) { Remove-Item $Exe -Force }
    # Remove PATH
    $p = ([Environment]::GetEnvironmentVariable("PATH","User") -split ";") -notlike "*Azkare\bin*" -join ";"
    [Environment]::SetEnvironmentVariable("PATH",$p,"User")
    # Remove autostart registry key
    $run = "HKCU:\Software\Microsoft\Windows\CurrentVersion\Run"
    Remove-ItemProperty $run -Name "Azkare" -ErrorAction SilentlyContinue
    $yn = Read-Host "Remove source and config? [y/N]"
    if ($yn -match "^[Yy]$") {
        Remove-Item $Root -Recurse -Force -ErrorAction SilentlyContinue
        Remove-Item "$env:APPDATA\Azkare" -Recurse -Force -ErrorAction SilentlyContinue
    }
    Write-Ok "Azkare uninstalled."
    exit 0
}

# ── Auto-install build deps ────────────────────────────────────────────────────
function Install-Deps {
    Write-I "Checking build dependencies..."

    # Detect available package manager
    $PM = if (Has "winget") { "winget" } elseif (Has "choco") { "choco" } elseif (Has "scoop") { "scoop" } else { $null }

    function PM-Install { param($WingetId,$ChocoId,$ScoopId,$Name)
        switch ($PM) {
            "winget" { winget install --id $WingetId --silent --accept-package-agreements --accept-source-agreements 2>$null }
            "choco"  { choco install $ChocoId -y --no-progress 2>$null }
            "scoop"  { scoop install $ScoopId 2>$null }
            default  { Write-W "Cannot auto-install $Name — please install manually." }
        }
        Refresh-Path
    }

    if (-not (Has "git")) {
        Write-I "Auto-installing Git..."
        PM-Install "Git.Git" "git" "git" "Git"
    } else { Write-Ok "Git OK" }

    if (-not (Has "cmake")) {
        Write-I "Auto-installing CMake..."
        PM-Install "Kitware.CMake" "cmake" "cmake" "CMake"
    } else { Write-Ok "CMake OK" }

    # C++ compiler: check for cl.exe or g++
    if (-not (Has "cl") -and -not (Has "g++")) {
        Write-I "Auto-installing C++ Build Tools..."
        if ($PM -eq "winget") {
            winget install --id Microsoft.VisualStudio.2022.BuildTools --silent `
                --override "--quiet --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended" `
                --accept-package-agreements --accept-source-agreements 2>$null
        } elseif ($PM -eq "choco") {
            choco install visualstudio2022buildtools -y --no-progress 2>$null
        } else {
            Write-W "Install MSVC Build Tools from: https://aka.ms/vs/17/release/vs_BuildTools.exe"
        }
        Refresh-Path
    } else { Write-Ok "C++ compiler OK" }

    # Qt6 — try to find existing installation first
    $QtDir = Get-ChildItem "C:\Qt" -Filter "Qt6Config.cmake" -Recurse -ErrorAction SilentlyContinue |
             Select-Object -First 1
    if (-not $QtDir) {
        Write-I "Auto-installing Qt6..."
        if ($PM -eq "winget") {
            winget install --id TheQtCompany.QtOnline --silent --accept-package-agreements 2>$null
        } elseif ($PM -eq "choco") {
            choco install qt6 -y --no-progress 2>$null
        } else {
            Write-W "Qt6 not found. Download from https://www.qt.io/download-open-source"
            Write-W "Then re-run this script."
        }
        Refresh-Path
    } else { Write-Ok "Qt6 found at: $(Split-Path $QtDir.FullName -Parent)" }

    Write-Ok "Dependencies satisfied"
}

# ── Download Arabic fonts ──────────────────────────────────────────────────────
function Get-Fonts {
    $FontDir = "$SrcDir\resources\fonts"
    New-Item -ItemType Directory -Path $FontDir -Force | Out-Null
    Write-I "Downloading open-source Arabic fonts..."

    $fonts = @{
        "Amiri-Regular.ttf"            = "https://github.com/aliftype/amiri/releases/latest/download/Amiri-Regular.ttf"
        "Amiri-Bold.ttf"               = "https://github.com/aliftype/amiri/releases/latest/download/Amiri-Bold.ttf"
        "ScheherazadeNew-Regular.ttf"  = "https://github.com/silnrsi/font-scheherazade/releases/latest/download/ScheherazadeNew-Regular.ttf"
        "NotoNaskhArabic-Regular.ttf"  = "https://github.com/notofonts/arabic/releases/latest/download/NotoNaskhArabic-Regular.ttf"
        "IBMPlexSansArabic-Regular.ttf"= "https://github.com/IBM/plex/raw/master/IBM-Plex-Sans-Arabic/fonts/complete/ttf/IBMPlexSansArabic-Regular.ttf"
        "ArefRuqaa-Regular.ttf"        = "https://github.com/google/fonts/raw/main/ofl/arefruqaa/ArefRuqaa-Regular.ttf"
    }

    foreach ($f in $fonts.GetEnumerator()) {
        $dest = "$FontDir\$($f.Key)"
        if (-not (Test-Path $dest)) {
            try {
                Invoke-WebRequest -Uri $f.Value -OutFile $dest -UseBasicParsing -TimeoutSec 30
                Write-Ok "  $($f.Key)"
            } catch { Write-W "  $($f.Key) — download failed" }
        } else { Write-Ok "  $($f.Key) (cached)" }
    }
}

# ── Clone or update ────────────────────────────────────────────────────────────
function Get-Source {
    if (Test-Path "$SrcDir\.git") {
        Write-I "Pulling latest source..."
        git -C $SrcDir fetch --quiet origin
        git -C $SrcDir reset --hard --quiet origin/main
    } else {
        Write-I "Cloning Azkare..."
        New-Item -ItemType Directory -Path (Split-Path $SrcDir) -Force | Out-Null
        git clone --depth=1 --quiet $Repo $SrcDir
    }
    $ver = Get-Content "$SrcDir\VERSION" -ErrorAction SilentlyContinue
    Write-Ok "Source ready — version $ver"
}

# ── Build ──────────────────────────────────────────────────────────────────────
function Build-Az {
    Write-I "Configuring & building (Release)..."
    $BuildDir = "$SrcDir\build"
    New-Item -ItemType Directory -Path $BuildDir -Force | Out-Null

    $QtCmake = Get-ChildItem "C:\Qt" -Filter "Qt6Config.cmake" -Recurse -ErrorAction SilentlyContinue |
               Select-Object -First 1
    $QtArg = if ($QtCmake) { "-DCMAKE_PREFIX_PATH=`"$(Split-Path $QtCmake.FullName -Parent)`"" } else { "" }

    & cmake -B $BuildDir -S $SrcDir -DCMAKE_BUILD_TYPE=Release $QtArg -Wno-dev 2>&1 |
        Out-File "$BuildDir\_cmake.log"
    if ($LASTEXITCODE -ne 0) { Write-E "CMake configure failed. See $BuildDir\_cmake.log" }

    & cmake --build $BuildDir --config Release --parallel 2>&1 |
        Out-File "$BuildDir\_build.log"
    if ($LASTEXITCODE -ne 0) { Write-E "Build failed. See $BuildDir\_build.log" }

    Write-Ok "Build complete"
    return $BuildDir
}

# ── Install binary ─────────────────────────────────────────────────────────────
function Install-Az { param($BuildDir)
    New-Item -ItemType Directory -Path $BinDir -Force | Out-Null
    $built = Get-ChildItem $BuildDir -Recurse -Filter "azkare.exe" -ErrorAction SilentlyContinue |
             Select-Object -First 1
    if (-not $built) { Write-E "azkare.exe not found after build." }
    Copy-Item $built.FullName $Exe -Force
    Add-UserPath $BinDir
    Write-Ok "Installed → $Exe"
}

# ── Main ───────────────────────────────────────────────────────────────────────
if ($Update) {
    Get-Source
    $b = Build-Az
    Install-Az $b
    Write-Ok "Updated. Run: azkare --version"
} else {
    Install-Deps
    Get-Source
    Get-Fonts
    $b = Build-Az
    Install-Az $b
    Write-Host ""
    Write-Host "✓ Azkare installed!" -ForegroundColor Green
    Write-Host "  azkare               — start" -ForegroundColor Cyan
    Write-Host "  azkare --settings    — open settings" -ForegroundColor Cyan
    Write-Host "  azkare --update      — update" -ForegroundColor Cyan
    Write-Host "  azkare --uninstall   — remove" -ForegroundColor Cyan
    Write-Host "  azkare --help        — full CLI reference" -ForegroundColor Cyan
    Write-W "Restart your terminal for PATH to take effect."
}
