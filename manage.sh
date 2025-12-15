#!/bin/bash

# Chemins de base
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$SCRIPT_DIR"
BUILD_DIR="$ROOT_DIR/build"
EXECUTABLE="$BUILD_DIR/WeedlyWeb"
INSTALL_DIR="/usr/local/bin"

# Fonction de nettoyage
clean_project() {
    echo "🧹 Nettoyage du répertoire de build et des fichiers racine CMake..."
    rm -rf "$BUILD_DIR" "$ROOT_DIR/CMakeCache.txt" "$ROOT_DIR/CMakeFiles" "$ROOT_DIR/Makefile" "$ROOT_DIR/cmake_install.cmake"
    echo "✔️ Nettoyage terminé."
}

# Fonction de nettoyage forcé
clean_force() {
    echo "🧹 Nettoyage forcé (supprime tout le cache CMake)..."
    rm -rf "$BUILD_DIR" "$ROOT_DIR/CMakeCache.txt" "$ROOT_DIR/CMakeFiles" "$ROOT_DIR/Makefile" "$ROOT_DIR/cmake_install.cmake"
    # Nettoyer aussi les fichiers cachés dans build/
    find "$BUILD_DIR" -name "CMakeCache.txt" -delete 2>/dev/null || true
    find "$BUILD_DIR" -name "CMakeFiles" -type d -exec rm -rf {} + 2>/dev/null || true
    echo "✔️ Nettoyage forcé terminé."
}

# Fonction de configuration et de build
build_project() {
    echo "🔨 Construction du projet..."
    mkdir -p "$BUILD_DIR"
    
    # Vérifier et nettoyer le cache CMake si nécessaire
    if [ -f "$BUILD_DIR/CMakeCache.txt" ]; then
        CACHED_SOURCE=$(grep "^CMAKE_HOME_DIRECTORY:" "$BUILD_DIR/CMakeCache.txt" 2>/dev/null | cut -d= -f2 | tr -d '\n')
        CURRENT_SOURCE=$(pwd)
        if [ "$CACHED_SOURCE" != "$CURRENT_SOURCE" ] && [ -n "$CACHED_SOURCE" ]; then
            echo "⚠️  Cache CMake détecté depuis un autre répertoire, nettoyage..."
            rm -rf "$BUILD_DIR/CMakeCache.txt" "$BUILD_DIR/CMakeFiles"
        fi
    fi
    
    cd "$BUILD_DIR" || exit 1
    echo "⚙️ Génération des fichiers de build avec CMake..."
    if ! cmake ..; then
        echo "❌ Erreur lors de la configuration CMake."
        echo "💡 Essayez: ./manage.sh clean build"
        exit 1
    fi
    echo "⚒️ Compilation avec make..."
    if ! make -j$(nproc); then
        echo "❌ Erreur lors de la compilation."
        exit 1
    fi
    echo "✔️ Build terminé avec succès."
    cd "$ROOT_DIR"
}

# Fonction d'exécution standard
run_project() {
    build_project
    echo "🚀 Lancement de WeedlyWeb (Mode Debug)..."
    gnome-terminal -- bash -c "$EXECUTABLE; exec bash" &
    echo "✅ WeedlyWeb lancé avec le PID : $!"
}

# Fonction de débogage avec GDB
debug_project() {
    build_project
    echo "🐞 Lancement en mode debug avec GDB..."
    gnome-terminal -- bash -c "gdb $EXECUTABLE; exec bash" &
}

# Fonction de monitoring avec possibilité de recompiler
monitor_project() {
    echo "📈 Surveillance active (appuyez sur 'r' pour recompiler et relancer)..."
    while true; do
        clear
        #PID=$(pgrep -x WeedlyWeb)
        PID=$(pgrep -f "$EXECUTABLE")
        if [ -z "$PID" ]; then
            echo "⚠️ Aucun processus détecté. En attente..."
        else
            echo "✅ Processus détecté : PID = $PID"
            ps -p "$PID" -o pid,%cpu,%mem,cmd
        fi

        # Attente d'une touche pressée
        read -t 2 -n 1 key
        if [[ "$key" == "r" ]]; then
            echo "🔄 Recompilation en cours..."
            kill "$PID"
            build_project
            run_project
        fi
    done
}


# Fonction de monitoring avancé avec btop (optionnel)
monitor_project_advanced() {
    echo "📊 Lancement de btop pour un suivi détaillé..."
    PID=$(pgrep -x WeedlyWeb)
    if [ -z "$PID" ]; then
        echo "⚠️ Aucun processus WeedlyWeb détecté. En attente..."
    else
        btop -C "$PID"
    fi
}

# Fonction pour exécuter et surveiller dans deux terminaux séparés
run_and_monitor() {
    build_project
    echo "🚀 Lancement de WeedlyWeb avec monitoring dans une autre fenêtre..."
    
    # Lancement de WeedlyWeb dans un terminal
    gnome-terminal -- bash -c "$EXECUTABLE; exec bash" &
    sleep 2  # Attente pour le démarrage correct du processus

    # Lancement du monitoring dans un autre terminal
    gnome-terminal -- bash -c "\"$SCRIPT_DIR/manage.sh\" monitor_project; exec bash" &
}

# Fonction d'installation
install_project() {
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
    echo "  clean                 Nettoyage complet du projet"
    echo "  build                 Compilation complète"
    echo "  run_project           Build et exécution du projet"
    echo "  debug                 Build et lancement avec GDB"
    echo "  monitor project       Surveillance basique du processus"
    echo "  monitor_project_advanced Utiliser btop pour une surveillance avancée"
    echo "  run_and_monitor       Compile, exécute et surveille le projet"
    echo "  install               Installer l'application"
    echo "  help                  Afficher ce message d'aide"
    echo ""
    echo "Exemples :"
    echo "  $0 clean build run_project"  
    echo "  $0 build debug"          
    echo "  $0 run_and_monitor"
    echo "  $0 install"
    echo "  $0 clean build run_and_monitor"
}

# Gestion des options
if [ $# -eq 0 ]; then
    show_help
    exit 0
fi

for arg in "$@"; do
    case $arg in
        clean) clean_project ;;
        clean-force) clean_force ;;
        build) build_project ;;
        run_project) run_project ;;
        debug) debug_project ;;
        monitor_project) monitor_project ;;
        monitor_project_advanced) monitor_project_advanced ;;
        run_and_monitor) run_and_monitor ;;
        install) install_project ;;
        help) show_help ;;
        *) echo "❌ Option inconnue : $arg"; show_help; exit 1 ;;
    esac
done
