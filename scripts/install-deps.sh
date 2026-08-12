#!/usr/bin/env bash
# Automatic dependency installation for WeedlyWeb
# Arch/Manjaro, Debian/Ubuntu/Lubuntu/Xubuntu, Fedora, openSUSE
set -euo pipefail

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m'

NONINTERACTIVE="${WEEDLYWEB_NONINTERACTIVE:-0}"
INSTALL_DEBUG=false
INSTALL_MONITOR=false

echo -e "${BLUE}🌐 WeedlyWeb - Installation des dépendances${NC}"
echo ""

if command -v pacman >/dev/null 2>&1; then
    DISTRO="arch"
    INSTALL_CMD=(sudo pacman -S --needed --noconfirm)
    DEPS_BUILD="cmake base-devel pkg-config"
    DEPS_MAIN="webkit2gtk gtk3 sqlite curl gdk-pixbuf2"
    DEPS_DEBUG="gdb valgrind strace"
    DEPS_MONITOR="inotify-tools"
    # Backends affichage (souvent déjà présents)
    DEPS_DISPLAY="wayland libx11"
    DEPS_QT="qt6-base qt6-webengine"
elif command -v apt-get >/dev/null 2>&1; then
    DISTRO="debian"
    INSTALL_CMD=(sudo apt-get install -y)
    DEPS_BUILD="cmake build-essential pkg-config"
    # Ubuntu / Lubuntu / Xubuntu / Linux Mint / Pop!_OS
    DEPS_MAIN="libwebkit2gtk-4.1-dev libgtk-3-dev libsqlite3-dev libcurl4-openssl-dev libgdk-pixbuf2.0-dev"
    DEPS_DEBUG="gdb valgrind strace"
    DEPS_MONITOR="inotify-tools"
    DEPS_DISPLAY="libwayland-client0 libx11-6 xwayland"
    DEPS_QT="qt6-base-dev qt6-webengine-dev"
elif command -v dnf >/dev/null 2>&1; then
    DISTRO="fedora"
    INSTALL_CMD=(sudo dnf install -y)
    DEPS_BUILD="cmake gcc-c++ pkg-config"
    DEPS_MAIN="webkit2gtk4.1-devel gtk3-devel sqlite-devel libcurl-devel gdk-pixbuf2-devel"
    # Fallback ancien nom si 4.1 absent
    DEPS_MAIN_FALLBACK="webkit2gtk4-devel gtk3-devel sqlite-devel libcurl-devel gdk-pixbuf2-devel"
    DEPS_DEBUG="gdb valgrind strace"
    DEPS_MONITOR="inotify-tools"
    DEPS_DISPLAY="libwayland-client libX11 xorg-x11-server-Xwayland"
    DEPS_QT="qt6-qtbase-devel qt6-qtwebengine-devel"
elif command -v zypper >/dev/null 2>&1; then
    DISTRO="opensuse"
    INSTALL_CMD=(sudo zypper install -y)
    DEPS_BUILD="cmake gcc-c++ pkg-config"
    DEPS_MAIN="webkit2gtk3-devel gtk3-devel sqlite3-devel libcurl-devel gdk-pixbuf-devel"
    DEPS_DEBUG="gdb valgrind strace"
    DEPS_MONITOR="inotify-tools"
    DEPS_DISPLAY="libwayland-client0 libX11-6 xwayland"
    DEPS_QT="qt6-base-devel qt6-webengine-devel"
else
    echo -e "${RED}❌ Distribution non supportée automatiquement${NC}"
    echo "Installez manuellement les paquets (voir docs/INSTALL_DEPENDENCIES.md"
    echo "et docs/COMPATIBILITY.md)."
    exit 1
fi

echo -e "${GREEN}✅ Distribution détectée: ${DISTRO}${NC}"
echo ""

ask_yes() {
    local prompt="$1"
    if [[ "$NONINTERACTIVE" == "1" ]]; then
        return 0
    fi
    read -r -p "$prompt (O/n) " -n 1 reply || true
    echo ""
    [[ -z "${reply:-}" || "${reply}" =~ ^[OoYy]$ ]]
}

if ask_yes "$(echo -e "${YELLOW}Installer les outils de débogage (GDB, Valgrind, strace)?${NC}")"; then
    INSTALL_DEBUG=true
fi

if ask_yes "$(echo -e "${YELLOW}Installer les outils de surveillance (inotify-tools pour make dev)?${NC}")"; then
    INSTALL_MONITOR=true
fi

INSTALL_QT=false
if ask_yes "$(echo -e "${YELLOW}Installer Qt6 WebEngine (backend src/ui/qt)?${NC}")"; then
    INSTALL_QT=true
fi

DEPS="$DEPS_BUILD $DEPS_MAIN ${DEPS_DISPLAY:-}"
[[ "$INSTALL_DEBUG" == true ]] && DEPS="$DEPS $DEPS_DEBUG"
[[ "$INSTALL_MONITOR" == true ]] && DEPS="$DEPS $DEPS_MONITOR"
[[ "$INSTALL_QT" == true && -n "${DEPS_QT:-}" ]] && DEPS="$DEPS $DEPS_QT"

echo ""
echo -e "${YELLOW}Paquets prévus:${NC}"
# shellcheck disable=SC2086
echo $DEPS | tr ' ' '\n' | sed 's/^/  - /'
echo ""

if [[ "$NONINTERACTIVE" != "1" ]]; then
    if ! ask_yes "Continuer l'installation?"; then
        echo -e "${YELLOW}Installation annulée${NC}"
        exit 0
    fi
fi

if [[ "$DISTRO" == "debian" ]]; then
    echo -e "${YELLOW}📦 Mise à jour de la liste des paquets...${NC}"
    sudo apt-get update
fi

echo -e "${YELLOW}🔧 Installation en cours...${NC}"
# shellcheck disable=SC2086
if ! "${INSTALL_CMD[@]}" $DEPS; then
    if [[ -n "${DEPS_MAIN_FALLBACK:-}" ]]; then
        echo -e "${YELLOW}⚠️  Nouvelle tentative avec paquets Fedora alternatifs…${NC}"
        DEPS_FALL="$DEPS_BUILD $DEPS_MAIN_FALLBACK ${DEPS_DISPLAY:-}"
        [[ "$INSTALL_DEBUG" == true ]] && DEPS_FALL="$DEPS_FALL $DEPS_DEBUG"
        [[ "$INSTALL_MONITOR" == true ]] && DEPS_FALL="$DEPS_FALL $DEPS_MONITOR"
        # shellcheck disable=SC2086
        "${INSTALL_CMD[@]}" $DEPS_FALL
    else
        exit 1
    fi
fi

echo ""
echo -e "${GREEN}✅ Installation terminée!${NC}"
echo ""

echo -e "${YELLOW}Vérification:${NC}"
if pkg-config --exists webkit2gtk-4.1 2>/dev/null; then
    echo -e "${GREEN}✅ WebKit2GTK 4.1: OK${NC}"
elif pkg-config --exists webkit2gtk-4.0 2>/dev/null; then
    echo -e "${GREEN}✅ WebKit2GTK 4.0: OK${NC}"
else
    echo -e "${RED}❌ WebKit2GTK: MANQUANT${NC}"
fi
if pkg-config --exists gtk+-3.0 2>/dev/null; then
    echo -e "${GREEN}✅ GTK+3: OK${NC}"
else
    echo -e "${RED}❌ GTK+3: MANQUANT${NC}"
fi
if pkg-config --exists sqlite3 2>/dev/null; then
    echo -e "${GREEN}✅ SQLite3: OK${NC}"
else
    echo -e "${RED}❌ SQLite3: MANQUANT${NC}"
fi
if command -v cmake >/dev/null 2>&1; then
    echo -e "${GREEN}✅ CMake: OK${NC}"
else
    echo -e "${RED}❌ CMake: MANQUANT${NC}"
fi

echo ""
echo -e "${GREEN}Compilez puis lancez avec le lanceur multi-backend:${NC}"
echo -e "${BLUE}   make run${NC}"
echo -e "${BLUE}   # ou ./scripts/run-weedlyweb.sh${NC}"
echo -e "Compatibilité DE/distros : docs/COMPATIBILITY.md"
