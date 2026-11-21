# أذكاري — Azkare

**أذكار وأدعية أصيلة على خلفية سطح مكتبك**  
*Authentic Adhkar & Duas on your desktop background*

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20Windows%20%7C%20macOS-informational)](https://github.com/arcalc/azkare/releases)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-orange.svg)](CMakeLists.txt)
[![Qt6](https://img.shields.io/badge/Qt-6.x-green.svg)](CMakeLists.txt)

---

## التثبيت / Install

### Linux & macOS — أمر واحد
```bash
curl -fsSL https://raw.githubusercontent.com/arcalc/azkare/main/install.sh | bash
```
يكتشف توزيعتك (`pacman` / `apt` / `dnf` / `zypper` / `apk` / `brew`)، يثبّت التبعيات تلقائيًا، يُنزّل الخطوط العربية، يبني من المصدر، ويثبّت في `~/.local/bin/`.

### Windows (PowerShell)
```powershell
irm https://raw.githubusercontent.com/arcalc/azkare/main/install.ps1 | iex
```
يثبّت Git وCMake وأدوات C++ وQt6 عبر `winget` ثم يبني ويثبّت في `%LOCALAPPDATA%\Azkare\bin\`.

---

## CLI Reference / مرجع الأوامر

| Command | Description |
|---------|-------------|
| `azkare` | Start overlay + system tray |
| `azkare --settings` | Open bilingual settings GUI |
| `azkare --next` | Rotate to next Dhikr immediately |
| `azkare --mode <overlay\|notify\|both>` | Switch display mode |
| `azkare --autostart <on\|off>` | Enable/disable login autostart |
| `azkare --update` | Pull latest source, rebuild, restart |
| `azkare --uninstall` | Remove binary, autostart, and optionally config |
| `azkare --status` | Show running status |
| `azkare --version` | Print `vX.Y.Z (githash, built YYYY-MM-DD)` |
| `azkare --help` | Bilingual (AR/EN) usage guide |

---

## Versioning & Releases / الإصدارات

Single source of truth: **`VERSION`** file in repo root.

```bash
# Bump version, commit, tag, and push (triggers GitHub Actions CI release)
./scripts/release.sh patch   # 1.0.0 → 1.0.1
./scripts/release.sh minor   # 1.0.0 → 1.1.0
./scripts/release.sh major   # 1.0.0 → 2.0.0
./scripts/release.sh 1.2.3   # explicit version
```

GitHub Actions (`.github/workflows/release.yml`) automatically builds for Linux, Windows, and macOS and publishes to [Releases](https://github.com/arcalc/azkare/releases) when a `v*` tag is pushed.

---

## Building from Source / البناء من المصدر

```bash
git clone https://github.com/arcalc/azkare.git && cd azkare
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel $(nproc)
sudo cmake --install build
```

**Requirements:** CMake ≥ 3.20, Qt6 (Widgets, Network), C++17 compiler, Git

---

## Project Structure

```
azkare/
├── VERSION                        ← single version source of truth
├── CMakeLists.txt                 ← reads VERSION, generates Version.h
├── install.sh                     ← zero-touch Linux/macOS installer
├── install.ps1                    ← zero-touch Windows installer
├── scripts/release.sh             ← version bump + git tag + push
├── cmake/Version.h.in             ← template for auto-generated Version.h
├── .github/workflows/release.yml  ← CI: build + GitHub Release on v* tag
├── src/
│   ├── main.cpp                   ← full CLI parser + IPC server
│   ├── core/                      ← DB, settings, scheduler, updater
│   ├── overlay/                   ← Qt desktop layer + eww bridge
│   ├── notifications/             ← D-Bus / Toast / UNUserNotification
│   ├── platform/                  ← autostart, tray, macOS bridges
│   └── ui/                        ← bilingual settings GUI
├── resources/
│   ├── data/adhkar.json           ← 500+ Adhkar (Dorar, IslamWeb, IslamQA)
│   ├── fonts/fonts_manifest.json
│   └── i18n/ar.json  en.json
├── packaging/linux/azkare.desktop
└── website/                       ← GitHub Pages site (terminal-only UI)
```

---

## Developer

**Mahmoud El Sheikh (arcalc)**  
🌐 https://arcalc.github.io/arcalc · 💻 https://github.com/arcalc · 📧 arcalc@proton.me

MIT License
