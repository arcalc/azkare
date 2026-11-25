#!/usr/bin/env bash
# ══════════════════════════════════════════════════════════════════════════════
# Azkare (أذكاري) — terminal installer for Linux and macOS
#
#   curl -fsSL https://raw.githubusercontent.com/arcalc/azkare/main/install.sh | bash
#
#   bash install.sh [--update | --uninstall [--purge]] [--autostart | --no-autostart]
#
# Environment overrides:  AZKARE_REF=v1.0.1  (install a tag/branch)   AZKARE_REPO=<git url or path>
# Compatible with bash 3.2 (macOS). Safe to run repeatedly.
# ══════════════════════════════════════════════════════════════════════════════
# shellcheck disable=SC1091,SC2016
set -eu

REPO="${AZKARE_REPO:-https://github.com/arcalc/azkare.git}"
REF="${AZKARE_REF:-main}"
SRC_DIR="${AZKARE_SRC:-$HOME/.local/share/azkare-src}"
PREFIX="$HOME/.local"
BIN_DIR="$PREFIX/bin"
BIN="$BIN_DIR/azkare"
BUILD_DIR="$SRC_DIR/build"

ACTION=install
PURGE=0
AUTOSTART=ask

if [ -t 1 ]; then
    C_CYAN=$(printf '\033[0;36m'); C_GREEN=$(printf '\033[0;32m'); C_YELLOW=$(printf '\033[1;33m')
    C_RED=$(printf '\033[0;31m'); C_BOLD=$(printf '\033[1m'); C_RST=$(printf '\033[0m')
else
    C_CYAN=""; C_GREEN=""; C_YELLOW=""; C_RED=""; C_BOLD=""; C_RST=""
fi
info() { printf '%s[azkare]%s %s\n' "$C_CYAN" "$C_RST" "$*"; }
ok()   { printf '%s[ok]%s %s\n' "$C_GREEN" "$C_RST" "$*"; }
warn() { printf '%s[!]%s %s\n' "$C_YELLOW" "$C_RST" "$*" >&2; }
die()  { printf '%s[x]%s %s\n' "$C_RED" "$C_RST" "$*" >&2; exit 1; }

usage() {
    sed -n '2,12p' "$0" 2>/dev/null | sed 's/^# \{0,1\}//' || true
}

for arg in "$@"; do
    case "$arg" in
        --update) ACTION=update ;;
        --uninstall) ACTION=uninstall ;;
        --purge) PURGE=1 ;;
        --autostart) AUTOSTART=yes ;;
        --no-autostart) AUTOSTART=no ;;
        install|--install) ACTION=install ;;
        -h|--help) usage; exit 0 ;;
        *) die "Unknown option: $arg (try --help)" ;;
    esac
done

# ask "question"  -> returns 0 for yes. Reads /dev/tty so it also works under `curl | bash`.
ask() {
    answer=""
    if [ -r /dev/tty ] && [ -w /dev/tty ]; then
        printf '%s [y/N] ' "$1" > /dev/tty
        read -r answer < /dev/tty 2>/dev/null || answer=""
    fi
    case "$answer" in y|Y|yes|YES) return 0 ;; *) return 1 ;; esac
}

# ── OS / package manager ─────────────────────────────────────────────────────
detect_os() {
    ARCH="$(uname -m)"
    if [ "$(uname -s)" = "Darwin" ]; then
        OS=macos; PM=brew
        command -v brew >/dev/null 2>&1 || die "Homebrew is required: https://brew.sh"
        return
    fi
    OS_ID=""; OS_LIKE=""
    if [ -r /etc/os-release ]; then
        OS_ID="$(. /etc/os-release; echo "${ID:-}")"
        OS_LIKE="$(. /etc/os-release; echo "${ID_LIKE:-}")"
    fi
    case " $OS_ID $OS_LIKE " in
        *" arch "*|*" manjaro "*|*" endeavouros "*|*" garuda "*) PM=pacman ;;
        *" debian "*|*" ubuntu "*) PM=apt ;;
        *" fedora "*|*" rhel "*|*" centos "*) PM=dnf ;;
        *" suse "*|*" opensuse "*|*" opensuse-leap "*|*" opensuse-tumbleweed "*) PM=zypper ;;
        *" alpine "*) PM=apk ;;
        *) PM=unknown ;;
    esac
    OS=linux
}

if [ "$(id -u)" -eq 0 ]; then SUDO=""; elif command -v sudo >/dev/null 2>&1; then SUDO="sudo"; else SUDO=""; fi

# Install packages; never aborts the script for optional ones.
pm_install() { # <required|optional> pkg...
    kind="$1"; shift
    case "$PM" in
        pacman) cmd="$SUDO pacman -S --needed --noconfirm" ;;
        apt)    cmd="$SUDO env DEBIAN_FRONTEND=noninteractive apt-get install -y" ;;
        dnf)    cmd="$SUDO dnf install -y" ;;
        zypper) cmd="$SUDO zypper --non-interactive install" ;;
        apk)    cmd="$SUDO apk add" ;;
        brew)   cmd="brew install" ;;
        *)      return 1 ;;
    esac
    # shellcheck disable=SC2086
    if $cmd "$@" >/dev/null 2>&1; then return 0; fi
    if [ "$PM" = apt ]; then   # package lists may be stale
        $SUDO apt-get update >/dev/null 2>&1 || true
        # shellcheck disable=SC2086
        $cmd "$@" >/dev/null 2>&1 && return 0
    fi
    if [ "$kind" = required ]; then
        # re-run visibly so the user sees why it failed
        # shellcheck disable=SC2086
        $cmd "$@" || die "Could not install: $*"
    fi
    return 1
}

install_deps() {
    info "Installing build dependencies (${PM})..."
    case "$PM" in
        pacman) pm_install required git cmake ninja gcc make curl fontconfig qt6-base libx11
                pm_install optional qt6-wayland layer-shell-qt || true ;;
        apt)    pm_install required git cmake ninja-build g++ make curl fontconfig qt6-base-dev libx11-dev
                pm_install optional qt6-wayland liblayershellqtinterface-dev || true ;;
        dnf)    pm_install required git cmake ninja-build gcc-c++ make curl fontconfig qt6-qtbase-devel libX11-devel
                pm_install optional qt6-qtwayland layer-shell-qt-devel || true ;;
        zypper) pm_install required git cmake ninja gcc-c++ make curl fontconfig qt6-base-devel libX11-devel
                pm_install optional qt6-wayland layer-shell-qt6-devel || true ;;
        apk)    pm_install required git cmake samurai g++ make curl fontconfig qt6-qtbase-dev libx11-dev
                pm_install optional qt6-qtwayland || true ;;
        brew)   pm_install required git cmake ninja qt curl ;;
        *)      for t in git cmake curl; do command -v "$t" >/dev/null 2>&1 || die "Unsupported distro: please install git, cmake, a C++17 compiler, Qt6 (Widgets, Network) and curl, then re-run."; done
                warn "Unknown package manager: assuming Qt6 development files are installed." ;;
    esac
    for t in git cmake curl; do command -v "$t" >/dev/null 2>&1 || die "$t is still missing after installation."; done
    ok "Dependencies ready"
}

# ── Source ───────────────────────────────────────────────────────────────────
fetch_source() {
    mkdir -p "$(dirname "$SRC_DIR")"
    if [ -d "$SRC_DIR/.git" ]; then
        info "Updating source ($REF)..."
        git -C "$SRC_DIR" fetch --quiet --depth=1 origin "$REF" || die "git fetch failed"
        git -C "$SRC_DIR" reset --hard --quiet FETCH_HEAD
    elif [ -e "$SRC_DIR" ]; then
        die "$SRC_DIR exists but is not a git checkout. Move it away and re-run."
    else
        info "Cloning Azkare ($REF)..."
        git clone --quiet --depth=1 --branch "$REF" "$REPO" "$SRC_DIR" || die "git clone failed ($REPO @ $REF)"
    fi
    ok "Source: version $(cat "$SRC_DIR/VERSION" 2>/dev/null || echo unknown)"
}

fetch_fonts() {
    info "Fetching Arabic fonts..."
    if bash "$SRC_DIR/scripts/fetch-fonts.sh" >/dev/null 2>&1; then ok "Fonts ready"
    else warn "Some fonts could not be downloaded; Azkare will fall back to system fonts."; fi
}

build() {
    info "Building (Release) - this takes a few minutes..."
    mkdir -p "$BUILD_DIR"
    EXTRA=""
    if [ "$OS" = macos ]; then EXTRA="-DCMAKE_PREFIX_PATH=$(brew --prefix qt)"; fi
    # shellcheck disable=SC2086
    cmake -S "$SRC_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release -DAZKARE_FETCH_FONTS=OFF $EXTRA \
        > "$BUILD_DIR/configure.log" 2>&1 || { tail -n 25 "$BUILD_DIR/configure.log" >&2; die "CMake configure failed (log: $BUILD_DIR/configure.log)"; }
    JOBS="$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2)"
    cmake --build "$BUILD_DIR" --parallel "$JOBS" \
        > "$BUILD_DIR/build.log" 2>&1 || { tail -n 25 "$BUILD_DIR/build.log" >&2; die "Build failed (log: $BUILD_DIR/build.log)"; }
    [ -x "$BUILD_DIR/azkare" ] || die "Build finished but $BUILD_DIR/azkare is missing."
    ok "Build complete"
}

ensure_path() {
    case ":$PATH:" in *":$BIN_DIR:"*) return 0 ;; esac
    added=0
    for rc in "$HOME/.bashrc" "$HOME/.zshrc"; do
        if [ -f "$rc" ] && ! grep -q '# azkare-installer' "$rc" 2>/dev/null; then
            printf '\n# azkare-installer\nexport PATH="$HOME/.local/bin:$PATH"\n' >> "$rc"
            info "Added ~/.local/bin to PATH in $rc"; added=1
        fi
    done
    if [ -d "$HOME/.config/fish" ] && ! grep -q '# azkare-installer' "$HOME/.config/fish/config.fish" 2>/dev/null; then
        printf '\n# azkare-installer\nfish_add_path $HOME/.local/bin\n' >> "$HOME/.config/fish/config.fish"; added=1
    fi
    if [ "$added" -eq 0 ] && ! grep -q '# azkare-installer' "$HOME/.profile" 2>/dev/null; then
        printf '\n# azkare-installer\nexport PATH="$HOME/.local/bin:$PATH"\n' >> "$HOME/.profile"
        info "Added ~/.local/bin to PATH in ~/.profile"
    fi
    export PATH="$BIN_DIR:$PATH"
    NEED_NEW_SHELL=1
}

install_bin() {
    mkdir -p "$BIN_DIR"
    # copy + rename: replacing a running binary in place would fail with "text file busy"
    install -m 755 "$BUILD_DIR/azkare" "$BIN.new" && mv -f "$BIN.new" "$BIN"
    if [ "$OS" = linux ]; then
        mkdir -p "$HOME/.local/share/applications" "$HOME/.local/share/icons/hicolor/scalable/apps"
        sed "s|^Exec=.*|Exec=$BIN|" "$SRC_DIR/packaging/linux/azkare.desktop" > "$HOME/.local/share/applications/azkare.desktop"
        cp "$SRC_DIR/packaging/linux/azkare.svg" "$HOME/.local/share/icons/hicolor/scalable/apps/azkare.svg"
    fi
    ensure_path
    ok "Installed $BIN ($("$BIN" --version 2>/dev/null || echo '?'))"
}

maybe_autostart() {
    if [ "$AUTOSTART" = ask ]; then
        if ask "Start Azkare automatically when you log in?"; then AUTOSTART=yes; else AUTOSTART=no; fi
    fi
    if [ "$AUTOSTART" = yes ]; then "$BIN" --autostart on && ok "Autostart enabled"; fi
}

do_uninstall() {
    info "Uninstalling Azkare..."
    if [ -x "$BIN" ]; then "$BIN" --autostart off >/dev/null 2>&1 || true; "$BIN" --quit >/dev/null 2>&1 || true; fi
    rm -f "$BIN" "$BIN.new" \
          "$HOME/.local/share/applications/azkare.desktop" \
          "$HOME/.local/share/icons/hicolor/scalable/apps/azkare.svg" \
          "$HOME/.config/autostart/azkare.desktop" \
          "$HOME/Library/LaunchAgents/io.github.arcalc.azkare.plist"
    for rc in "$HOME/.bashrc" "$HOME/.zshrc" "$HOME/.profile" "$HOME/.config/fish/config.fish"; do
        if [ -f "$rc" ] && grep -q '# azkare-installer' "$rc"; then
            awk '/# azkare-installer/ { skip = 2 } skip > 0 { skip--; next } { print }' "$rc" > "$rc.azkare-tmp" && mv "$rc.azkare-tmp" "$rc"
        fi
    done
    if [ "$PURGE" -eq 0 ] && ask "Also delete your settings and the source/build folder?"; then PURGE=1; fi
    if [ "$PURGE" -eq 1 ]; then
        rm -rf "$SRC_DIR" "$HOME/.config/Azkare" "$HOME/Library/Preferences/Azkare"
        ok "Settings and source removed"
    else
        info "Kept: $SRC_DIR and ~/.config/Azkare (use --purge to delete them)"
    fi
    ok "Azkare uninstalled"
}

# ── main ─────────────────────────────────────────────────────────────────────
NEED_NEW_SHELL=0
printf '%s\n  أذكاري — Azkare installer\n%s\n' "$C_BOLD" "$C_RST"
detect_os
info "Platform: $OS/$ARCH ${PM:+(package manager: $PM)}"

case "$ACTION" in
    uninstall) do_uninstall ;;
    update)
        fetch_source; fetch_fonts; build; install_bin
        ok "Updated. Run: azkare --version" ;;
    install)
        install_deps; fetch_source; fetch_fonts; build; install_bin; maybe_autostart
        printf '\n%sAzkare is installed.%s\n' "$C_BOLD$C_GREEN" "$C_RST"
        printf '  azkare              start in the background\n  azkare --settings   open settings\n  azkare --update     update\n  azkare --uninstall  remove\n  azkare --help       everything else\n'
        if [ "$NEED_NEW_SHELL" -eq 1 ]; then warn "Open a new terminal (or run: export PATH=\"\$HOME/.local/bin:\$PATH\") to use 'azkare'."; fi ;;
esac
