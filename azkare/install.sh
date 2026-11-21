#!/usr/bin/env bash
# ═══════════════════════════════════════════════════════════════════════════════
# Azkare (أذكاري) — Zero-Touch Terminal Installer  |  Linux & macOS
#
#   curl -fsSL https://raw.githubusercontent.com/arcalc/azkare/main/install.sh | bash
#   bash install.sh --update
#   bash install.sh --uninstall
# ═══════════════════════════════════════════════════════════════════════════════
set -euo pipefail

REPO="https://github.com/arcalc/azkare.git"
RAW="https://raw.githubusercontent.com/arcalc/azkare/main"
SRC_DIR="${HOME}/.local/share/azkare-src"
BIN_DIR="${HOME}/.local/bin"
BIN="${BIN_DIR}/azkare"
PREFIX="${HOME}/.local"
BUILD_DIR="${SRC_DIR}/build"
ACTION="${1:-install}"

# ── ANSI helpers ──────────────────────────────────────────────────────────────
C_CYAN='\033[0;36m'; C_GREEN='\033[0;32m'; C_YELLOW='\033[1;33m'
C_RED='\033[0;31m';  C_BOLD='\033[1m';     C_RST='\033[0m'
info()    { echo -e "${C_CYAN}[أذكاري]${C_RST} $*"; }
ok()      { echo -e "${C_GREEN}[✓]${C_RST} $*"; }
warn()    { echo -e "${C_YELLOW}[!]${C_RST} $*"; }
die()     { echo -e "${C_RED}[✗]${C_RST} $*" >&2; exit 1; }

banner() {
echo -e "${C_BOLD}${C_CYAN}"
echo "  ╔═══════════════════════════════════╗"
echo "  ║   أذكاري — Azkare  Installer     ║"
echo "  ║   arcalc.github.io/azkare        ║"
echo "  ╚═══════════════════════════════════╝"
echo -e "${C_RST}"
}

# ── Detect OS / distro / arch ─────────────────────────────────────────────────
detect_os() {
    ARCH="$(uname -m)"
    if [[ "$OSTYPE" == "darwin"* ]]; then
        OS="macos"; PM="brew"
        command -v brew &>/dev/null || die "Homebrew required: https://brew.sh"
        return
    fi
    [[ -f /etc/os-release ]] && source /etc/os-release || true
    case "${ID:-}" in
        arch|manjaro|endeavouros|garuda) OS="arch";    PM="pacman" ;;
        ubuntu|debian|linuxmint|pop|kali) OS="debian"; PM="apt" ;;
        fedora)                           OS="fedora";  PM="dnf" ;;
        rhel|centos|rocky|almalinux)      OS="rhel";   PM="dnf" ;;
        opensuse*|suse*)                  OS="opensuse";PM="zypper" ;;
        alpine)                           OS="alpine";  PM="apk" ;;
        *)                                OS="generic"; PM="unknown" ;;
    esac
}

# ── Auto-install a package (silent) ──────────────────────────────────────────
pkg_install() {
    local pkgs=("$@")
    info "Auto-installing: ${pkgs[*]}"
    case "$PM" in
        pacman)  sudo pacman -S --needed --noconfirm "${pkgs[@]}" ;;
        apt)     sudo apt-get install -y -qq "${pkgs[@]}" ;;
        dnf)     sudo dnf install -y -q "${pkgs[@]}" ;;
        zypper)  sudo zypper install -y --quiet "${pkgs[@]}" ;;
        apk)     sudo apk add --quiet "${pkgs[@]}" ;;
        brew)    brew install "${pkgs[@]}" >/dev/null ;;
        *)       warn "Unknown package manager — install manually: ${pkgs[*]}" ;;
    esac
}

# ── Check command, install if missing ────────────────────────────────────────
need() {
    local cmd="$1"; shift
    command -v "$cmd" &>/dev/null || pkg_install "$@"
}

# ── Install all build + runtime deps ─────────────────────────────────────────
install_deps() {
    info "Checking and auto-installing build dependencies..."
    case "$PM" in
        pacman)
            PKGS=()
            for p in git cmake ninja qt6-base qt6-tools curl libnotify fontconfig gcc; do
                pacman -Qq "$p" &>/dev/null || PKGS+=("$p")
            done
            [[ ${#PKGS[@]} -gt 0 ]] && sudo pacman -S --needed --noconfirm "${PKGS[@]}"
            ;;
        apt)
            sudo apt-get update -qq
            PKGS=()
            for p in git cmake ninja-build qt6-base-dev libqt6network6-dev \
                      curl libnotify-dev libfontconfig1-dev build-essential; do
                dpkg -s "$p" &>/dev/null || PKGS+=("$p")
            done
            [[ ${#PKGS[@]} -gt 0 ]] && sudo apt-get install -y -qq "${PKGS[@]}"
            ;;
        dnf)
            for p in git cmake ninja-build qt6-qtbase-devel \
                      curl libnotify-devel fontconfig-devel gcc-c++; do
                rpm -q "$p" &>/dev/null || sudo dnf install -y -q "$p"
            done
            ;;
        zypper)
            for p in git cmake ninja qt6-base-devel curl \
                      libnotify-devel fontconfig-devel gcc-c++; do
                rpm -q "$p" &>/dev/null || sudo zypper install -y --quiet "$p"
            done
            ;;
        apk)
            sudo apk add --quiet git cmake ninja qt6-dev curl \
                 libnotify-dev fontconfig-dev g++ make
            ;;
        brew)
            for p in git cmake ninja qt6 curl; do
                brew list "$p" &>/dev/null || brew install "$p" >/dev/null
            done
            export PATH="$(brew --prefix qt6)/bin:$PATH"
            ;;
        *)
            warn "Cannot auto-install deps — ensure git, cmake, qt6, curl are installed."
            ;;
    esac
    ok "Dependencies satisfied"
}

# ── Download bundled Arabic fonts ─────────────────────────────────────────────
download_fonts() {
    local FONT_DIR="${SRC_DIR}/resources/fonts"
    mkdir -p "$FONT_DIR"
    info "Downloading open-source Arabic fonts..."

    local -A FONTS=(
        ["Amiri-Regular.ttf"]="https://github.com/aliftype/amiri/releases/latest/download/Amiri-Regular.ttf"
        ["Amiri-Bold.ttf"]="https://github.com/aliftype/amiri/releases/latest/download/Amiri-Bold.ttf"
        ["ScheherazadeNew-Regular.ttf"]="https://github.com/silnrsi/font-scheherazade/releases/latest/download/ScheherazadeNew-Regular.ttf"
        ["NotoNaskhArabic-Regular.ttf"]="https://github.com/notofonts/arabic/releases/latest/download/NotoNaskhArabic-Regular.ttf"
        ["IBMPlexSansArabic-Regular.ttf"]="https://github.com/IBM/plex/raw/master/IBM-Plex-Sans-Arabic/fonts/complete/ttf/IBMPlexSansArabic-Regular.ttf"
        ["ArefRuqaa-Regular.ttf"]="https://github.com/google/fonts/raw/main/ofl/arefruqaa/ArefRuqaa-Regular.ttf"
    )

    for fname in "${!FONTS[@]}"; do
        local dest="${FONT_DIR}/${fname}"
        if [[ ! -f "$dest" ]]; then
            curl -fsSL --retry 3 --retry-delay 2 -o "$dest" "${FONTS[$fname]}" 2>/dev/null \
                && ok "  ${fname}" \
                || warn "  ${fname} — download failed (using system fallback)"
        else
            ok "  ${fname} (cached)"
        fi
    done

    # Refresh font cache
    command -v fc-cache &>/dev/null && fc-cache -f 2>/dev/null || true
}

# ── Clone or update ───────────────────────────────────────────────────────────
fetch_source() {
    if [[ -d "${SRC_DIR}/.git" ]]; then
        info "Pulling latest source..."
        git -C "$SRC_DIR" fetch --quiet origin
        git -C "$SRC_DIR" reset --hard --quiet origin/main
    else
        info "Cloning Azkare..."
        git clone --depth=1 --quiet "$REPO" "$SRC_DIR"
    fi
    local ver; ver=$(cat "${SRC_DIR}/VERSION" 2>/dev/null || echo "unknown")
    ok "Source ready — version ${ver}"
}

# ── Build ─────────────────────────────────────────────────────────────────────
build() {
    info "Configuring & building (Release)..."
    local CMAKE_EXTRA=()
    [[ "$OS" == "macos" ]] && CMAKE_EXTRA+=("-DCMAKE_PREFIX_PATH=$(brew --prefix qt6)")

    cmake -B "$BUILD_DIR" -S "$SRC_DIR" \
          -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_INSTALL_PREFIX="$PREFIX" \
          "${CMAKE_EXTRA[@]}" \
          -Wno-dev \
          > "${BUILD_DIR}_cmake.log" 2>&1 \
        || die "CMake configure failed — see ${BUILD_DIR}_cmake.log"

    local JOBS; JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2)
    cmake --build "$BUILD_DIR" --parallel "$JOBS" \
          > "${BUILD_DIR}_build.log" 2>&1 \
        || die "Build failed — see ${BUILD_DIR}_build.log"

    ok "Build complete"
}

# ── Install binary + PATH ─────────────────────────────────────────────────────
install_bin() {
    mkdir -p "$BIN_DIR"
    cmake --install "$BUILD_DIR" --prefix "$PREFIX" > /dev/null 2>&1 || true

    # Ensure ~/.local/bin is on PATH for future shells
    for rc in "${HOME}/.bashrc" "${HOME}/.zshrc" "${HOME}/.profile"; do
        if [[ -f "$rc" ]] && ! grep -q 'HOME/.local/bin' "$rc" 2>/dev/null; then
            printf '\n# Added by Azkare installer\nexport PATH="$HOME/.local/bin:$PATH"\n' >> "$rc"
            info "Updated PATH in ${rc}"
            break
        fi
    done
    export PATH="$BIN_DIR:$PATH"

    # XDG .desktop
    local DESK="${HOME}/.local/share/applications"
    mkdir -p "$DESK"
    cp "${SRC_DIR}/packaging/linux/azkare.desktop" "$DESK/" 2>/dev/null || true

    ok "Installed → ${BIN}"
}

# ── Uninstall ─────────────────────────────────────────────────────────────────
do_uninstall() {
    info "Uninstalling Azkare..."
    rm -f "$BIN"
    rm -f "${HOME}/.local/share/applications/azkare.desktop"
    rm -f "${HOME}/.config/autostart/azkare.desktop"
    # Remove PATH line we added
    for rc in "${HOME}/.bashrc" "${HOME}/.zshrc" "${HOME}/.profile"; do
        [[ -f "$rc" ]] && sed -i '/# Added by Azkare installer/,+1d' "$rc" 2>/dev/null || true
    done
    if [[ -t 0 ]]; then
        read -rp "Also remove source and config? [y/N] " yn
        [[ "$yn" =~ ^[Yy]$ ]] && rm -rf "$SRC_DIR" "${HOME}/.config/Azkare" "${HOME}/.local/share/Azkare"
    fi
    ok "Azkare uninstalled."
    exit 0
}

# ── Entry ─────────────────────────────────────────────────────────────────────
banner
detect_os
info "Detected: ${OS} (${ARCH}), package manager: ${PM}"

case "$ACTION" in
    --uninstall)
        do_uninstall ;;
    --update)
        fetch_source
        build
        install_bin
        ok "Azkare updated. Run: azkare --version" ;;
    install|--install|"")
        install_deps
        fetch_source
        download_fonts
        build
        install_bin
        echo ""
        echo -e "${C_BOLD}${C_GREEN}✓ Azkare installed successfully!${C_RST}"
        echo -e "  ${C_CYAN}azkare${C_RST}               — start"
        echo -e "  ${C_CYAN}azkare --settings${C_RST}    — open settings"
        echo -e "  ${C_CYAN}azkare --update${C_RST}      — update"
        echo -e "  ${C_CYAN}azkare --uninstall${C_RST}   — remove"
        echo -e "  ${C_CYAN}azkare --help${C_RST}        — full CLI reference"
        [[ ":$PATH:" != *":$BIN_DIR:"* ]] && \
            warn "Restart your terminal (or run: source ~/.bashrc) to use 'azkare'" ;;
    *)
        die "Unknown action: $ACTION" ;;
esac
