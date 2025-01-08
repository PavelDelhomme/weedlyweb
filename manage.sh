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
    rm -rf "$BUILD_DIR" "$ROOT_DIR/CMakeCache.txt" "$ROOT_DIR/CMakeFiles" "$ROOT_DIR/Makefile" "$ROOT_DIR/cmake_install.cmake"
    
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
    build_project
    echo "🚀 Lancement de l'application (mode Debug + Monitoring)..."

    # Lancer le programme dans un terminal séparé pour monitoring
    gnome-terminal -- bash -c "$EXECUTABLE; exec bash" &

    # Surveillance des ressources dans une autre fenêtre
    gnome-terminal -- bash -c "watch -n 2 'ps -p $(pgrep -f WeedlyWeb) -o pid,%cpu,%mem,cmd'; exec bash" &
}

# Fonction de débogage
debug_project() {
    build_project
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
    build_project
    echo "📦 Installation de l'application..."
    sudo make install
    echo "✔️ Application installée avec succès."
}

# Fonction d'affichage de l'aide
show_help() {
    echo "🔧 Utilisation : $0 [OPTION]"
    echo ""
    echo "Options disponibles :"
    echo "  clean         Nettoyage complet du projet"
    echo "  build         Compilation complète"
    echo "  run_project   Build, Run et Monitoring"
    echo "  debug         Build et lancement avec GDB"
    echo "  monitor       Surveillance des ressources"
    echo "  install       Installer le projet"
    echo "  help          Afficher ce message d'aide"
    echo ""
    echo "Exemples :"
    echo "  $0 clean build run_project"  
    echo "  $0 build debug"          
    echo "  $0 install"
}

# Gestion des options
if [ $# -eq 0 ]; then
    show_help
    exit 0
fi

for arg in "$@"; do
    case $arg in
        clean) clean_project ;;
        build) build_project ;;
        run_project) run_project ;;
        debug) debug_project ;;
        monitor) monitor_project ;;
        install) install_project ;;
        help) show_help ;;
        *) echo "❌ Option inconnue : $arg"; show_help; exit 1 ;;
    esac
done
