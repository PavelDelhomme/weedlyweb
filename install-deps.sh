#!/bin/bash
# Script d'installation automatique des dépendances pour WeedlyWeb
# Compatible avec Manjaro/Arch Linux

set -e

# Couleurs
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${YELLOW}📦 Installation des dépendances pour WeedlyWeb${NC}"
echo ""

# Vérifier si on est sur Arch/Manjaro
if ! command -v pacman >/dev/null 2>&1; then
    echo -e "${RED}❌ Ce script est conçu pour Arch Linux / Manjaro${NC}"
    echo "Pour d'autres distributions, consultez INSTALL_DEPENDENCIES.md"
    exit 1
fi

# Demander si on veut installer les outils de débogage
echo -e "${YELLOW}Voulez-vous installer les outils de débogage (GDB, Valgrind, strace) ?${NC}"
echo -e "${YELLOW}Ils sont optionnels mais recommandés pour le développement.${NC}"
read -p "Installer les outils de débogage ? (O/n) " -n 1 -r
echo ""

if [[ $REPLY =~ ^[OoYy]$ ]] || [[ -z $REPLY ]]; then
    INSTALL_DEBUG=true
else
    INSTALL_DEBUG=false
fi

# Dépendances obligatoires
DEPS="cmake webkit2gtk gtk3 sqlite curl base-devel pkg-config"

# Ajouter les outils de débogage si demandé
if [ "$INSTALL_DEBUG" = true ]; then
    DEPS="$DEPS gdb valgrind strace"
    echo -e "${GREEN}Installation complète (avec outils de débogage)${NC}"
else
    echo -e "${GREEN}Installation minimale (sans outils de débogage)${NC}"
fi

echo ""
echo -e "${YELLOW}Les packages suivants seront installés :${NC}"
echo "$DEPS" | tr ' ' '\n' | sed 's/^/  - /'
echo ""

read -p "Continuer l'installation ? (O/n) " -n 1 -r
echo ""

if [[ ! $REPLY =~ ^[OoYy]$ ]] && [[ ! -z $REPLY ]]; then
    echo -e "${YELLOW}Installation annulée${NC}"
    exit 0
fi

# Installation
echo -e "${YELLOW}🔧 Installation en cours...${NC}"
sudo pacman -S --needed $DEPS

echo ""
echo -e "${GREEN}✅ Installation terminée !${NC}"
echo ""
echo -e "${YELLOW}Vérification de l'installation :${NC}"
make check-deps

echo ""
echo -e "${GREEN}🎉 Vous pouvez maintenant compiler le projet avec : make build${NC}"

