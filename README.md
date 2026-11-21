# أذكاري — Azkare

**أذكار وأدعية أصيلة على خلفية سطح مكتبك**  
*Authentic Adhkar & Duas on your desktop background*

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20Windows%20%7C%20macOS-informational)](https://github.com/arcalc/azkare/releases)
[![C++17](https://img.shields.io/badge/C%2B%2B-17%2F20-orange.svg)](CMakeLists.txt)
[![Qt6](https://img.shields.io/badge/Qt-6.x-green.svg)](CMakeLists.txt)

---

<div dir="rtl">

## ما هو أذكاري؟

**أذكاري** هو تطبيق مكتبي مفتوح المصدر مكتوب بالكامل بـ C++17 مع Qt6، يعرض أكثر من **٥٠٠ ذكر ودعاء أصيل** مباشرةً فوق خلفية سطح مكتبك — **تحت جميع نوافذ تطبيقاتك** — مع إشعارات دورية تُذكّرك بذكر الله طوال يومك.

صُمّم أذكاري **خصيصًا للمستخدم العادي**: كل إعداد مرفق بشرح واضح بلغة بسيطة، ولا يحتاج التطبيق أي معرفة تقنية لاستخدامه.

</div>

---

## Features / المميزات

| Feature | التفاصيل |
|---------|----------|
| 🖥️ **Desktop Wallpaper Overlay** | Renders Dhikr text above wallpaper, below all app windows — no focus stealing |
| 🔔 **Native Notifications** | libnotify/D-Bus (Linux), Toast (Windows), UNUserNotification (macOS) |
| 📖 **500+ Verified Adhkar** | Sourced from Dorar.net, IslamWeb, IslamQA — with hadith references |
| 🔤 **Bundled Arabic Fonts** | Amiri, Scheherazade, Noto Naskh, Cairo, IBM Plex Arabic, Aref Ruqaa |
| 🌐 **Bilingual (Arabic-first)** | Full RTL Arabic + LTR English — switch with one click |
| ⏱️ **Flexible Timer** | 30s, 1min, 5min, 1hr or any custom interval |
| 🎨 **Full Appearance Control** | Font, size, color, shadow, alignment, position |
| 🐧 **eww Support (Linux)** | Auto-generates eww widget config & communicates via IPC |
| 🚀 **Autostart** | XDG (Linux) · Registry (Windows) · LaunchAgent (macOS) |
| 🔄 **GitHub Auto-Updater** | Checks latest release, shows notes, provides direct download |
| 🗂️ **Category Filtering** | Morning · Evening · Sleep · Post-Prayer · Istighfar · General · Quranic |
| 🔀 **Shuffle Mode** | Random or sequential Dhikr rotation |
| 📌 **System Tray** | Next Dhikr · Pause/Resume · Settings · Updates · Quit |

---

## Screenshots

> *Coming after first release — the official website at https://arcalc.github.io/azkare previews the UI mockup.*

---

## Building from Source / البناء من المصدر

### Requirements / المتطلبات

- **CMake** ≥ 3.20
- **Qt** 6.4+ (Widgets, Network, DBus on Linux)
- **C++17** compatible compiler (GCC 10+, Clang 12+, MSVC 2019+)
- **nlohmann/json** (fetched automatically by CMake)
- **SQLite3** (system package or bundled amalgamation)

**Linux extras (optional):**
- `libx11-dev` — X11 desktop-level window hints
- `wayland-protocols` — Wayland layer shell support
- `pkg-config`

### Clone & Build

```bash
git clone https://github.com/arcalc/azkare.git
cd azkare

# Download bundled Arabic fonts (see resources/fonts/fonts_manifest.json)
# Place .ttf files in resources/fonts/ before building

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel $(nproc)

# Run
./build/azkare
```

### Linux — AppImage

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build --parallel
DESTDIR=AppDir cmake --install build
# Then use linuxdeploy or appimagetool
```

### Windows (MSVC + vcpkg)

```powershell
vcpkg install qt6-base sqlite3
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=...vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
```

### macOS

```bash
brew install qt6 sqlite3
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=$(brew --prefix qt6)
cmake --build build --parallel
```

---

## Project Structure / هيكل المشروع

```
azkare/
├── CMakeLists.txt              # Root build config
├── src/
│   ├── main.cpp                # Entry point
│   ├── core/
│   │   ├── AdhkarDatabase.{h,cpp}   # JSON DB loader, iterator, shuffle
│   │   ├── AppSettings.{h,cpp}      # JSON-persisted settings
│   │   ├── Scheduler.{h,cpp}        # Timer loop — rotation + notifications
│   │   └── Updater.{h,cpp}          # GitHub releases API checker
│   ├── overlay/
│   │   ├── OverlayManager.{h,cpp}   # Backend selector (native vs eww)
│   │   ├── OverlayWidget.{h,cpp}    # Qt desktop-layer window + fade animation
│   │   └── EwwBridge.{h,cpp}        # Linux eww config writer + IPC
│   ├── notifications/
│   │   └── NotificationDispatcher.{h,cpp}  # Cross-platform notification sender
│   ├── platform/
│   │   ├── AutostartManager.{h,cpp}  # XDG / Registry / LaunchAgent
│   │   ├── TrayController.{h,cpp}    # System tray icon + menu
│   │   └── macos/
│   │       ├── MacOverlay.mm         # Obj-C++ NSWindow desktop level
│   │       └── MacNotifications.mm   # Obj-C++ UNUserNotificationCenter
│   └── ui/
│       └── SettingsWindow.{h,cpp}    # Bilingual tabbed settings GUI
├── resources/
│   ├── data/adhkar.json        # 500+ Adhkar dataset (Dorar, IslamWeb, IslamQA)
│   ├── fonts/fonts_manifest.json
│   └── i18n/
│       ├── ar.json             # Arabic strings (default)
│       └── en.json             # English strings
├── packaging/
│   └── linux/azkare.desktop    # XDG desktop entry
└── website/
    ├── index.html              # Official website (GitHub Pages)
    ├── styles.css
    └── app.js
```

---

## Adhkar Sources / مصادر الأذكار

All Adhkar in `resources/data/adhkar.json` are sourced from:

| Source | URL |
|--------|-----|
| **الدرر السنية** | https://dorar.net |
| **إسلام ويب** | https://islamweb.net |
| **الإسلام سؤال وجواب** | https://islamqa.info |

Each entry includes its `source_name_ar`, `ref` (hadith collection + number), and `url` back to the original source for verification.

---

## Overlay Z-Order — How It Works

```
┌─────────────────────────────────────────────┐
│  Z-layer 3 (top)   App windows (Firefox…)   │
├─────────────────────────────────────────────┤
│  Z-layer 2         Azkare Dhikr text ✨     │
├─────────────────────────────────────────────┤
│  Z-layer 1 (base)  Desktop wallpaper        │
└─────────────────────────────────────────────┘

Linux X11  — _NET_WM_WINDOW_TYPE_DESKTOP + XLowerWindow
Linux Wayland — wlr-layer-shell / gtk-layer-shell (bottom layer)
Windows    — WorkerW injection or WS_EX_LAYERED fallback
macOS      — NSWindow at kCGDesktopWindowLevel
```

---

## Configuration File

Settings are stored as JSON in the platform's standard config directory:

| Platform | Path |
|----------|------|
| Linux    | `~/.config/Azkare/settings.json` |
| Windows  | `%APPDATA%\Azkare\settings.json` |
| macOS    | `~/Library/Application Support/Azkare/settings.json` |

---

## Contributing / المساهمة

Pull requests are welcome! For major changes, please open an issue first.

1. Fork the repository
2. Create your branch: `git checkout -b feature/my-feature`
3. Commit your changes: `git commit -m 'Add my feature'`
4. Push and open a PR

**Adding more Adhkar:** Edit `resources/data/adhkar.json` following the existing schema and add your entry with its verified source reference.

---

## License / الرخصة

MIT License — see [LICENSE](LICENSE) for details.

---

## Developer / المطور

**Mahmoud El Sheikh (arcalc)**  
🌐 https://arcalc.github.io/arcalc  
💻 https://github.com/arcalc  
📧 arcalc@proton.me
