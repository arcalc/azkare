# أذكاري — Azkare

**أذكار وأدعية على سطح مكتبك** · *Adhkar & Duas on your desktop*

[![CI](https://github.com/arcalc/azkare/actions/workflows/ci.yml/badge.svg)](https://github.com/arcalc/azkare/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

Azkare shows Adhkar and Duas **above your wallpaper and below your windows**, and can send periodic
notifications. C++17 / Qt6. Installed and controlled entirely from the terminal.

يعرض أذكاري الأذكار والأدعية فوق خلفية سطح المكتب وتحت النوافذ، مع إشعارات دورية. مكتوب بـ C++17 وQt6،
ويُثبَّت ويُدار بالكامل من الطرفية.

---

## Install / التثبيت

```bash
# Linux & macOS
curl -fsSL https://raw.githubusercontent.com/arcalc/azkare/main/install.sh | bash
```
```powershell
# Windows (PowerShell)
irm https://raw.githubusercontent.com/arcalc/azkare/main/install.ps1 | iex
```

The installer installs build dependencies with your package manager (`pacman`, `apt`, `dnf`, `zypper`,
`apk`, `brew`; on Windows `winget`/`choco`/`scoop` + MSYS2), downloads the Arabic fonts, builds in
Release mode and installs `azkare` to `~/.local/bin` (`%LOCALAPPDATA%\Azkare\bin` on Windows).
Pin a version with `AZKARE_REF=v1.0.1`. Read the script first if you prefer: [`install.sh`](install.sh).

### Status — what has actually been tested

| Platform | Status |
|---|---|
| Linux, X11 (Ubuntu 24.04, Qt 6.4) | Built and run: overlay window type/stacking, IPC, settings window (AR + EN), install / update / uninstall |
| Linux, Wayland | Code path for `layer-shell-qt` exists but is **untested** (needs a wlroots/KDE compositor). On GNOME the overlay is an ordinary window |
| eww backend | Config generator written, **untested** |
| macOS | Code written, **not compiled or run yet** (CI job is experimental) |
| Windows | Code and MSYS2 installer written, **not compiled or run yet** (CI job is experimental) |

Please open an issue with logs if something fails on a platform marked untested.

## Usage / الاستخدام

```text
azkare                       start in the background (overlay + tray)
azkare --foreground          stay attached to the terminal
azkare --settings            open the settings window
azkare --next                show the next Dhikr now
azkare --mode overlay|notify|both
azkare --autostart on|off    start at login
azkare --update [--force]    check GitHub, pull, rebuild, restart
azkare --uninstall [--purge] remove (--purge also deletes settings and source)
azkare --quit                stop the running instance
azkare --status | --version | --help
```

All commands except starting and `--settings-window` work without a display (SSH-friendly).
Settings live in `settings.json`; `azkare --status` prints its path.

### Wayland
Build with `layer-shell-qt` installed (`layer-shell-qt` on Arch, `liblayershellqtinterface-dev` on
Debian/Ubuntu 24.10+). The installer tries to install it. Without it, Wayland sessions fall back to XWayland/X11 behaviour.

## Content / المحتوى

`resources/data/adhkar.json` (schema `azkare-adhkar-v2`) is a **starter set** (see `"total"`) in 7 categories. Every entry carries
`source_ar` **and** `source_en`; the overlay, notifications and eww backend always show both together
(`المصدر: … | Source: …`) whatever the interface language.

What is verified, and what is not (details per entry in [`AUDIT.md`](AUDIT.md)):

| | Quranic entries (11) | Prophetic adhkar (33) |
|---|---|---|
| Arabic text | verbatim from the Uthmani Hafs text, ayah by ayah | words matched against a Hisn al-Muslim-based text (30 of 33); **diacritics not independently verifiable** |
| English | verbatim Saheeh International | **unverified** (no source supplied) |
| Reference | surah:ayah verified | **hadith numbers unverified** |
| `count` | checked against the Hisn-based text | checked against the Hisn-based text |

Scholarly review is welcome. Re-run the checks with:

```bash
python3 scripts/validate_adhkar.py          # schema, bilingual sources, flags (offline)
bash scripts/fetch-audit-refs.sh && python3 scripts/audit_adhkar.py   # re-verifies every claim against the reference texts
```

## Build from source

```bash
git clone https://github.com/arcalc/azkare.git && cd azkare
bash scripts/fetch-fonts.sh          # optional: bundled Arabic fonts (SIL OFL) are fetched automatically if missing
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
sudo cmake --install build
```
Requires CMake ≥ 3.20, a C++17 compiler, Qt ≥ 6.2 (Widgets, Network; DBus optional), libX11 headers on Linux.
macOS: add `-DCMAKE_PREFIX_PATH="$(brew --prefix qt)"`.

## Releases / الإصدارات

`VERSION` is the single source of truth; CMake turns it plus the git hash and build date into `Version.h`.

```bash
./scripts/release.sh patch      # 1.0.0 -> 1.0.1   (also: minor | major | X.Y.Z)
```
It checks you are on a clean, up-to-date `main`, bumps `VERSION`, commits `chore(release): vX.Y.Z`, tags and pushes.
The tag triggers `.github/workflows/release.yml`, which checks tag == `VERSION`, validates data, test-builds and
publishes a GitHub Release with notes generated from the commits. `azkare --update` and the website read that release.

## Layout

```
VERSION  CMakeLists.txt  install.sh  install.ps1  LICENSE
cmake/Version.h.in         scripts/ (release, fetch-fonts, validate_adhkar, check_i18n)
src/core      dataset, settings, scheduler, updater, localizer, IPC
src/overlay   Qt overlay window (X11 / layer-shell / Win / mac) + eww bridge
src/ui        bilingual settings window      src/platform  autostart, tray, macOS bridge
src/notifications   D-Bus / Qt notifications
resources/    adhkar.json, i18n (ar, en), fonts manifest     docs/   website (GitHub Pages)
.github/      workflows (ci, release), SECURITY.md
```

## Website

`docs/` is served by GitHub Pages: **Settings → Pages → Deploy from a branch → `main` → `/docs`**.

## Developer

Mahmoud El Sheikh (**arcalc**) · https://arcalc.github.io/arcalc · arcalc@proton.me · MIT licence
