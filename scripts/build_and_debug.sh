#!/bin/bash

# Définir les variables pour les chemins
ROOT_DIR=$(pwd) # Répertoire racine du projet
BUILD_DIR="$ROOT_DIR/build" # Répertoire de build

# Nettoyage de l'ancien build
echo "Nettoyage de l'ancien build..."
if [ -d "$BUILD_DIR" ]; then
    rm -rf "$BUILD_DIR"
fi

# Création du répertoire build
mkdir -p "$BUILD_DIR"

# Aller dans le répertoire build
cd "$BUILD_DIR" || exit 1

# Configuration avec CMake
echo "Configuration du projet avec CMake..."
cmake ..

if [ $? -ne 0 ]; then
    echo "Erreur lors de la configuration CMake. Vérifiez vos fichiers CMakeLists.txt."
    exit 1
fi

# Compilation avec make
echo "Compilation du projet..."
make -j$(nproc)

if [ $? -ne 0 ]; then
    echo "Erreur lors de la compilation. Vérifiez les messages d'erreur."
    exit 1
fi

# Lancer le programme avec gdb pour le débogage
EXECUTABLE="$BUILD_DIR/WeedlyWeb"

if [ -f "$EXECUTABLE" ]; then
    cd "$BUILD_DIR" || exit 1
    echo "Lancement de l'application avec gdb..."
    gdb -ex "start" -ex "continue" "$EXECUTABLE"
else
    echo "Exécutable introuvable. Assurez-vous que la compilation s'est bien déroulée."
    exit 1
fi