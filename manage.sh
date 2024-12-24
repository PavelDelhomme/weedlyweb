#!/bin/bash

# Chemins de base
ROOT_DIR=$(pwd)
BUILD_DIR="$ROOT_DIR/build"
EXECUTABLE="$BUILD_DIR/WeedlyWeb"
INSTALL_DIR="/usr/local/bin"

# Fonction de nettoyage
clean_project() {
    echo "🧹 Nettoyage du répertoire de build et des fichiers racine CMake..."
    
    # Supprimer les fichiers générés par CMake dans le répertoire racine
    rm -f "$ROOT_DIR/CMakeCache.txt" "$ROOT_DIR/Makefile"
    rm -rf "$ROOT_DIR/CMakeFiles" "$ROOT_DIR/cmake_install.cmake"
    
    # Nettoyer le répertoire de build
    rm -rf "$BUILD_DIR"
    
    echo "✔️ Nettoyage terminé."
}

# Fonction de configuration et de build
build_project() {
    echo "🔨 Construction du projet..."
    
    # Créer le répertoire de build s'il n'existe pas
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR" || exit 1
    
    echo "⚙️ Génération des fichiers de build avec CMake..."
    if ! cmake ..; then
        echo "❌ Erreur lors de la configuration CMake. Vérifiez votre CMakeLists.txt."
        exit 1
    fi
    
    echo "⚒️ Compilation avec make..."
    if ! make -j$(nproc); then
        echo "❌ Erreur lors de la compilation."
        exit 1
    fi

    echo "✔️ Build terminé avec succès."
}

# Fonction d'exécution
run_project() {
    if [ ! -f "$EXECUTABLE" ]; then
        echo "❌ L'exécutable n'existe pas. Essayez de lancer le build d'abord."
        exit 1
    fi
    echo "🚀 Lancement de l'application..."
    "$EXECUTABLE"
}

# Fonction de débogage
debug_project() {
    if [ ! -f "$EXECUTABLE" ]; then
        echo "❌ L'exécutable n'existe pas. Essayez de lancer le build d'abord."
        exit 1
    fi
    echo "🐞 Lancement en mode debug avec gdb..."
    gdb "$EXECUTABLE"
}

# Fonction de monitoring
monitor_project() {
    PID=$(pgrep -f "WeedlyWeb")
    if [ -z "$PID" ]; then
        echo "❌ Aucun processus WeedlyWeb en cours d'exécution."
        exit 1
    fi
    echo "📈 Surveillance de WeedlyWeb (PID: $PID)"
    watch -n 2 "ps -p $PID -o pid,%cpu,%mem,cmd"
}

# Fonction d'installation (production)
install_project() {
    echo "📦 Installation en cours..."
    if [ ! -f "$EXECUTABLE" ]; then
        echo "❌ L'exécutable n'existe pas. Essayez de lancer le build d'abord."
        exit 1
    fi
    cd "$BUILD_DIR" || exit 1
    sudo make install
    echo "✔️ Installation réussie. Vous pouvez maintenant lancer le programme avec 'WeedlyWeb'."
}

# Fonction d'affichage de l'aide
show_help() {
    echo "🔧 Utilisation : $0 [OPTION]"
    echo ""
    echo "Options disponibles :"
    echo "  clean         Nettoyer le répertoire de build"
    echo "  build         Compiler le projet"
    echo "  run           Lancer le projet en mode normal"
    echo "  debug         Lancer le projet avec gdb pour le débogage"
    echo "  monitor       Surveiller les ressources utilisées par le programme"
    echo "  install       Installer le projet en mode production"
    echo "  help          Afficher ce message d'aide"
    echo ""
    echo "Exemples :"
    echo "  $0 clean build debug  # Nettoie, compile et lance en mode débogage"
    echo "  $0 build run          # Compile et lance en mode normal"
    echo "  $0 install            # Compile et installe pour une utilisation en production"
}

# Gestion des options
if [ $# -eq 0 ]; then
    show_help
    exit 0
fi

for arg in "$@"; do
    case $arg in
        clean)
            clean_project
            ;;
        build)
            build_project
            ;;
        run)
            run_project
            ;;
        debug)
            debug_project
            ;;
        monitor)
            monitor_project
            ;;
        install)
            install_project
            ;;
        help)
            show_help
            ;;
        *)
            echo "❌ Option invalide : $arg"
            show_help
            exit 1
            ;;
    esac
done
