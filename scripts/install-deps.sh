#!/bin/bash
# Automatic dependency installation script for WeedlyWeb
# Compatible with Manjaro/Arch Linux

set -e

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${YELLOW}📦 Installing dependencies for WeedlyWeb${NC}"
echo ""

# Check if we're on Arch/Manjaro
if ! command -v pacman >/dev/null 2>&1; then
    echo -e "${RED}❌ This script is designed for Arch Linux / Manjaro${NC}"
    echo "For other distributions, see INSTALL_DEPENDENCIES.md"
    exit 1
fi

# Ask if user wants to install debugging tools
echo -e "${YELLOW}Do you want to install debugging tools (GDB, Valgrind, strace)?${NC}"
echo -e "${YELLOW}They are optional but recommended for development.${NC}"
read -p "Install debugging tools? (Y/n) " -n 1 -r
echo ""

if [[ $REPLY =~ ^[Yy]$ ]] || [[ -z $REPLY ]]; then
    INSTALL_DEBUG=true
else
    INSTALL_DEBUG=false
fi

# Required dependencies
DEPS="cmake webkit2gtk gtk3 sqlite curl base-devel pkg-config"

# Add debugging tools if requested
if [ "$INSTALL_DEBUG" = true ]; then
    DEPS="$DEPS gdb valgrind strace"
    echo -e "${GREEN}Full installation (with debugging tools)${NC}"
else
    echo -e "${GREEN}Minimal installation (without debugging tools)${NC}"
fi

echo ""
echo -e "${YELLOW}The following packages will be installed:${NC}"
echo "$DEPS" | tr ' ' '\n' | sed 's/^/  - /'
echo ""

read -p "Continue installation? (Y/n) " -n 1 -r
echo ""

if [[ ! $REPLY =~ ^[Yy]$ ]] && [[ ! -z $REPLY ]]; then
    echo -e "${YELLOW}Installation cancelled${NC}"
    exit 0
fi

# Installation
echo -e "${YELLOW}🔧 Installing...${NC}"
sudo pacman -S --needed $DEPS

echo ""
echo -e "${GREEN}✅ Installation completed!${NC}"
echo ""
echo -e "${YELLOW}Verifying installation:${NC}"
make check-deps

echo ""
echo -e "${GREEN}🎉 You can now compile the project with: make build${NC}"
