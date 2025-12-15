#!/bin/bash
# Automatic dependency installation script for WeedlyWeb
# Compatible with Arch/Manjaro, Ubuntu/Debian, and Fedora

set -e

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}🌐 WeedlyWeb - Installation des dépendances${NC}"
echo ""

# Detect distribution
if command -v pacman >/dev/null 2>&1; then
    DISTRO="arch"
    PKG_MANAGER="pacman"
    INSTALL_CMD="sudo pacman -S --needed"
    DEPS_BUILD="cmake base-devel pkg-config"
    DEPS_MAIN="webkit2gtk gtk3 sqlite curl gdk-pixbuf2"
    DEPS_DEBUG="gdb valgrind strace"
    DEPS_MONITOR="inotify-tools"
elif command -v apt-get >/dev/null 2>&1; then
    DISTRO="debian"
    PKG_MANAGER="apt-get"
    INSTALL_CMD="sudo apt-get install -y"
    DEPS_BUILD="cmake build-essential pkg-config"
    DEPS_MAIN="libwebkit2gtk-4.1-dev libgtk-3-dev libsqlite3-dev libcurl4-openssl-dev libgdk-pixbuf2.0-dev"
    DEPS_DEBUG="gdb valgrind strace"
    DEPS_MONITOR="inotify-tools"
elif command -v dnf >/dev/null 2>&1; then
    DISTRO="fedora"
    PKG_MANAGER="dnf"
    INSTALL_CMD="sudo dnf install -y"
    DEPS_BUILD="cmake gcc-c++ pkg-config"
    DEPS_MAIN="webkit2gtk4-devel gtk3-devel sqlite-devel libcurl-devel gdk-pixbuf2-devel"
    DEPS_DEBUG="gdb valgrind strace"
    DEPS_MONITOR="inotify-tools"
else
    echo -e "${RED}❌ Distribution non supportée automatiquement${NC}"
    echo "Veuillez installer manuellement les dépendances. Voir docs/INSTALL_DEPENDENCIES.md"
    exit 1
fi

echo -e "${GREEN}✅ Distribution détectée: ${DISTRO^}${NC}"
echo ""

# Ask if user wants to install debugging tools
echo -e "${YELLOW}Voulez-vous installer les outils de débogage (GDB, Valgrind, strace)?${NC}"
echo -e "${YELLOW}Ils sont optionnels mais recommandés pour le développement.${NC}"
read -p "Installer les outils de débogage? (O/n) " -n 1 -r
echo ""

if [[ $REPLY =~ ^[OoYy]$ ]] || [[ -z $REPLY ]]; then
    INSTALL_DEBUG=true
else
    INSTALL_DEBUG=false
fi

# Ask if user wants to install monitoring tools
echo -e "${YELLOW}Voulez-vous installer les outils de surveillance (inotify-tools)?${NC}"
echo -e "${YELLOW}Nécessaires pour 'make dev' (recompilation automatique).${NC}"
read -p "Installer les outils de surveillance? (O/n) " -n 1 -r
echo ""

if [[ $REPLY =~ ^[OoYy]$ ]] || [[ -z $REPLY ]]; then
    INSTALL_MONITOR=true
else
    INSTALL_MONITOR=false
fi

# Build dependencies list
DEPS="$DEPS_BUILD $DEPS_MAIN"

# Add debugging tools if requested
if [ "$INSTALL_DEBUG" = true ]; then
    DEPS="$DEPS $DEPS_DEBUG"
    echo -e "${GREEN}Installation complète (avec outils de débogage)${NC}"
else
    echo -e "${GREEN}Installation minimale (sans outils de débogage)${NC}"
fi

# Add monitoring tools if requested
if [ "$INSTALL_MONITOR" = true ]; then
    DEPS="$DEPS $DEPS_MONITOR"
fi

echo ""
echo -e "${YELLOW}Les paquets suivants seront installés:${NC}"
echo "$DEPS" | tr ' ' '\n' | sed 's/^/  - /'
echo ""

read -p "Continuer l'installation? (O/n) " -n 1 -r
echo ""

if [[ ! $REPLY =~ ^[OoYy]$ ]] && [[ ! -z $REPLY ]]; then
    echo -e "${YELLOW}Installation annulée${NC}"
    exit 0
fi

# Update package list for Debian/Ubuntu
if [ "$DISTRO" = "debian" ]; then
    echo -e "${YELLOW}📦 Mise à jour de la liste des paquets...${NC}"
    sudo apt-get update
fi

# Installation
echo -e "${YELLOW}🔧 Installation en cours...${NC}"
$INSTALL_CMD $DEPS

echo ""
echo -e "${GREEN}✅ Installation terminée!${NC}"
echo ""

# Verify installation
echo -e "${YELLOW}Vérification de l'installation:${NC}"
if command -v make >/dev/null 2>&1; then
    make check-deps 2>/dev/null || echo -e "${YELLOW}⚠️  'make check-deps' non disponible, vérification manuelle...${NC}"
else
    echo -e "${YELLOW}⚠️  'make' non disponible, vérification manuelle...${NC}"
    echo ""
    echo -e "${BLUE}Vérification des dépendances principales:${NC}"
    pkg-config --exists webkit2gtk-4.1 && echo -e "${GREEN}✅ WebKit2GTK: OK${NC}" || echo -e "${RED}❌ WebKit2GTK: MANQUANT${NC}"
    pkg-config --exists sqlite3 && echo -e "${GREEN}✅ SQLite3: OK${NC}" || echo -e "${RED}❌ SQLite3: MANQUANT${NC}"
    pkg-config --exists gtk+-3.0 && echo -e "${GREEN}✅ GTK+3: OK${NC}" || echo -e "${RED}❌ GTK+3: MANQUANT${NC}"
    command -v cmake >/dev/null 2>&1 && echo -e "${GREEN}✅ CMake: OK${NC}" || echo -e "${RED}❌ CMake: MANQUANT${NC}"
fi

echo ""
echo -e "${GREEN}🎉 Vous pouvez maintenant compiler le projet avec:${NC}"
echo -e "${BLUE}   make build${NC}"
echo ""
echo -e "${GREEN}Ou lancer directement:${NC}"
echo -e "${BLUE}   make run${NC}"
