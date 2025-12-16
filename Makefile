# Makefile pour WeedlyWeb
# Browser web en C++ avec WebKit2GTK
#
# Ce Makefile utilise CMake pour générer le Makefile dans build/
# Workflow :
#   1. make configure -> CMake génère build/Makefile
#   2. make build     -> Utilise build/Makefile pour compiler
#
# Vous n'avez pas besoin de modifier ce Makefile, CMake gère tout !

# Variables
PROJECT_NAME := WeedlyWeb
BUILD_DIR := build
SOURCE_DIR := .
INSTALL_DIR := /usr/local
EXECUTABLE := $(BUILD_DIR)/$(PROJECT_NAME)
CMAKE := cmake
MAKE := make
NPROC := $(shell nproc 2>/dev/null || echo 4)

# Couleurs pour les messages
GREEN := \033[0;32m
YELLOW := \033[1;33m
RED := \033[0;31m
NC := \033[0m # No Color

.PHONY: all clean build run run-build run-bg run-debug build-debug debug debug-auto valgrind install uninstall reinstall help monitor test configure watch watch-run watch-basic watch-run-basic dev install-deps setup

# Cible par défaut - utiliser Qt si disponible, sinon GTK
all: build-qt

# Configuration Qt
configure-qt:
	@printf "$(YELLOW)⚙️  Configuration CMake pour Qt6...$(NC)\n"
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && $(CMAKE) -DUSE_QT=ON ..
	@printf "$(GREEN)✔️  Configuration Qt terminée$(NC)\n"

# Build Qt (prioritaire)
build-qt:
	@if pkg-config --exists Qt6Core Qt6Gui Qt6Widgets Qt6WebEngineWidgets 2>/dev/null; then \
		echo "$(GREEN)🔨 Compilation avec Qt6...$(NC)"; \
		cd ui-qt && mkdir -p build && cd build && $(CMAKE) .. && $(MAKE) -j$(NPROC) || (echo "$(RED)❌ Erreur de compilation Qt$(NC)"; exit 1); \
		echo "$(GREEN)✔️  Compilation Qt terminée$(NC)"; \
	else \
		echo "$(YELLOW)⚠️  Qt6 non disponible, utilisation de GTK...$(NC)"; \
		echo "$(YELLOW)💡 Pour installer Qt6 : sudo pacman -S qt6-base qt6-webengine$(NC)"; \
		$(MAKE) build-gtk; \
	fi

# Build GTK (fallback)
build-gtk: build

# Run Qt
run-qt: build-qt
	@printf "$(GREEN)🚀 Lancement de WeedlyWebQt...$(NC)\n"
	@if [ -f ui-qt/build/WeedlyWebQt ]; then \
		ui-qt/build/WeedlyWebQt; \
	else \
		printf "$(RED)❌ Erreur : L'exécutable Qt n'existe pas$(NC)\n"; \
		exit 1; \
	fi

# Afficher l'aide
help:
	@printf "$(GREEN)🔧 Makefile pour $(PROJECT_NAME)$(NC)\n"
	@printf "\n"
	@printf "$(YELLOW)Commandes disponibles :$(NC)\n"
	@printf "  $(GREEN)make$(NC)              - Compile le projet (alias de 'make build')\n"
	@printf "  $(GREEN)make build$(NC)        - Compile le projet\n"
	@printf "  $(GREEN)make clean$(NC)        - Nettoie le répertoire de build\n"
	@printf "  $(GREEN)make run$(NC)           - Compile et lance l'application (foreground)\n"
	@printf "  $(GREEN)make run-build$(NC)    - Compile puis lance l'application (vérifie le build)\n"
	@printf "  $(GREEN)make run-bg$(NC)       - Compile et lance en arrière-plan\n"
	@printf "  $(GREEN)make run-debug$(NC)    - Compile en debug et lance (avec symboles)\n"
	@printf "  $(GREEN)make debug$(NC)        - Compile en debug et lance avec GDB (interactif)\n"
	@printf "  $(GREEN)make debug-auto$(NC)   - Compile en debug et lance avec GDB (automatique)\n"
	@printf "  $(GREEN)make valgrind$(NC)     - Lance avec Valgrind (détection fuites)\n"
	@printf "  $(GREEN)make install$(NC)      - Installe l'application sur le système\n"
	@printf "  $(GREEN)make uninstall$(NC)    - Désinstalle l'application\n"
	@printf "  $(GREEN)make reinstall$(NC)    - Réinstalle l'application (uninstall + install)\n"
	@printf "  $(GREEN)make monitor$(NC)       - Surveille le processus en cours\n"
	@printf "  $(GREEN)make test$(NC)         - Lance les tests (si disponibles)\n"
	@printf "  $(GREEN)make configure$(NC)   - Configure CMake uniquement\n"
	@printf "  $(GREEN)make rebuild$(NC)      - Nettoie et recompile\n"
	@printf "  $(GREEN)make check-deps$(NC)   - Vérifie les dépendances installées\n"
	@printf "  $(GREEN)make watch$(NC)        - Surveille les fichiers et recompile automatiquement\n"
	@printf "  $(GREEN)make watch-run$(NC)    - Surveille, recompile et relance l'application automatiquement\n"
	@printf "  $(GREEN)make dev$(NC)          - Mode développement : surveille, recompile et recharge proprement l'application\n"
	@printf "\n"
	@printf "$(YELLOW)📦 Installation et configuration :$(NC)\n"
	@printf "  $(GREEN)make install-deps$(NC)  - Installe les dépendances (détection automatique de la distribution)\n"
	@printf "  $(GREEN)make setup$(NC)        - Configuration complète : installe les dépendances et compile\n"
	@printf "\n"
	@printf "$(YELLOW)💡 Documentation :$(NC)\n"
	@printf "  Voir $(GREEN)docs/INSTALL_DEPENDENCIES.md$(NC) pour les instructions détaillées\n"
	@printf "\n"
	@printf "$(YELLOW)📝 Note :$(NC)\n"
	@printf "  Ce Makefile utilise CMake pour générer le Makefile dans $(BUILD_DIR)/\n"
	@printf "  Vous n'avez pas besoin de modifier ce Makefile, CMake gère tout !\n"
	@printf "\n"

# Configuration CMake
# CMake génère automatiquement le Makefile dans build/
configure:
	@printf "$(YELLOW)⚙️  Configuration CMake (génération du Makefile dans build/)...$(NC)\n"
	@mkdir -p $(BUILD_DIR)
	@# Nettoyer le cache CMake si le répertoire source a changé
	@if [ -f $(BUILD_DIR)/CMakeCache.txt ]; then \
		CACHED_SOURCE=$$(grep "^CMAKE_HOME_DIRECTORY:" $(BUILD_DIR)/CMakeCache.txt 2>/dev/null | cut -d= -f2 | tr -d '\n'); \
		CURRENT_SOURCE=$$(pwd); \
		if [ "$$CACHED_SOURCE" != "$$CURRENT_SOURCE" ] && [ -n "$$CACHED_SOURCE" ]; then \
			printf "$(YELLOW)⚠️  Cache CMake détecté depuis un autre répertoire, nettoyage...$(NC)\n"; \
			rm -rf $(BUILD_DIR)/CMakeCache.txt $(BUILD_DIR)/CMakeFiles; \
		fi; \
	fi
	@cd $(BUILD_DIR) && $(CMAKE) ..
	@printf "$(GREEN)✔️  Configuration terminée - Makefile généré dans $(BUILD_DIR)/$(NC)\n"

# Compilation
# Utilise le Makefile généré par CMake dans build/
build: configure
	@printf "$(YELLOW)🔨 Compilation du projet (via Makefile généré par CMake)...$(NC)\n"
	@cd $(BUILD_DIR) && $(MAKE) -j$(NPROC)
	@printf "$(GREEN)✔️  Compilation terminée avec succès$(NC)\n"

# Nettoyage
clean:
	@echo "$(YELLOW)🧹 Nettoyage du projet...$(NC)"
	@rm -rf $(BUILD_DIR)
	@rm -f $(SOURCE_DIR)/CMakeCache.txt
	@rm -rf $(SOURCE_DIR)/CMakeFiles
	@# Ne pas supprimer le Makefile principal du projet
	@rm -f $(SOURCE_DIR)/cmake_install.cmake
	@echo "$(GREEN)✔️  Nettoyage terminé$(NC)"

# Nettoyage forcé (nettoie même si build/ existe)
clean-force: clean
	@echo "$(YELLOW)🧹 Nettoyage forcé...$(NC)"
	@rm -rf $(BUILD_DIR)/* $(BUILD_DIR)/.* 2>/dev/null || true
	@echo "$(GREEN)✔️  Nettoyage forcé terminé$(NC)"

# Rebuild complet
rebuild: clean build

# Exécution (mode normal - foreground pour voir les sorties)
# Redirige les erreurs GBM non-critiques vers /dev/null
run: build
	@printf "$(GREEN)🚀 Lancement de $(PROJECT_NAME)...$(NC)\n"
	@if [ -f $(EXECUTABLE) ]; then \
		$(EXECUTABLE) 2>/dev/null || $(EXECUTABLE); \
	else \
		printf "$(RED)❌ Erreur : L'exécutable n'existe pas$(NC)\n"; \
		exit 1; \
	fi

# Compilation puis exécution (vérifie que le build a réussi avant de lancer)
run-build:
	@printf "$(YELLOW)🔨 Compilation du projet...$(NC)\n"
	@if $(MAKE) build; then \
		printf "$(GREEN)✔️  Compilation réussie$(NC)\n"; \
		printf "$(GREEN)🚀 Lancement de $(PROJECT_NAME)...$(NC)\n"; \
		printf "$(YELLOW)💡 Les logs de débogage seront affichés ci-dessous$(NC)\n"; \
		printf "$(YELLOW)💡 Appuyez sur Ctrl+C pour arrêter$(NC)\n\n"; \
		if [ -f $(EXECUTABLE) ]; then \
			$(EXECUTABLE) 2>&1 || $(EXECUTABLE) 2>&1; \
		else \
			printf "$(RED)❌ Erreur : L'exécutable n'existe pas$(NC)\n"; \
			exit 1; \
		fi; \
	else \
		printf "$(RED)❌ Erreur de compilation - l'application ne sera pas lancée$(NC)\n"; \
		exit 1; \
	fi

# Exécution en arrière-plan
run-bg: build
	@echo "$(GREEN)🚀 Lancement de $(PROJECT_NAME) en arrière-plan...$(NC)"
	@if [ -f $(EXECUTABLE) ]; then \
		$(EXECUTABLE) & \
		PID=$$!; \
		echo "$(GREEN)✅ $(PROJECT_NAME) lancé avec le PID : $$PID$(NC)"; \
		echo "$(YELLOW)💡 Pour arrêter : kill $$PID$(NC)"; \
	else \
		echo "$(RED)❌ Erreur : L'exécutable n'existe pas$(NC)"; \
		exit 1; \
	fi

# Compilation en mode debug
build-debug:
	@echo "$(YELLOW)🔨 Compilation en mode debug...$(NC)"
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && $(CMAKE) -DCMAKE_BUILD_TYPE=Debug ..
	@cd $(BUILD_DIR) && $(MAKE) -j$(NPROC)
	@echo "$(GREEN)✔️  Compilation debug terminée$(NC)"

# Débogage avec GDB
debug: build-debug
	@echo "$(YELLOW)🐞 Lancement en mode debug avec GDB...$(NC)"
	@command -v gdb >/dev/null 2>&1 || { echo "$(RED)❌ GDB n'est pas installé. Installez-le avec : sudo pacman -S gdb$(NC)"; exit 1; }
	@if [ -f $(EXECUTABLE) ]; then \
		echo "$(GREEN)✅ Exécutable trouvé : $(EXECUTABLE)$(NC)"; \
		echo "$(YELLOW)💡 Commandes GDB utiles :$(NC)"; \
		echo "  $(GREEN)run$(NC)          - Lancer l'application"; \
		echo "  $(GREEN)bt$(NC)           - Afficher la stack trace (backtrace)"; \
		echo "  $(GREEN)continue$(NC)    - Continuer l'exécution"; \
		echo "  $(GREEN)quit$(NC)         - Quitter GDB"; \
		echo ""; \
		echo "$(YELLOW)🚀 Lancement de GDB...$(NC)"; \
		gdb -ex "set confirm off" -ex "set pagination off" $(EXECUTABLE); \
	else \
		echo "$(RED)❌ Erreur : L'exécutable n'existe pas$(NC)"; \
		exit 1; \
	fi

# Exécution en mode debug (sans GDB, juste avec symboles)
run-debug: build-debug
	@echo "$(GREEN)🚀 Lancement de $(PROJECT_NAME) en mode debug...$(NC)"
	@echo "$(YELLOW)💡 Appuyez sur Ctrl+C pour arrêter proprement$(NC)"
	@if [ -f $(EXECUTABLE) ]; then \
		trap 'echo ""; echo "$(YELLOW)🛑 Arrêt de l'application...$(NC)"; exit 0' INT TERM; \
		$(EXECUTABLE); \
	else \
		echo "$(RED)❌ Erreur : L'exécutable n'existe pas$(NC)"; \
		exit 1; \
	fi

# Débogage avec Valgrind (détection de fuites mémoire)
valgrind: build-debug
	@echo "$(YELLOW)🔍 Lancement avec Valgrind...$(NC)"
	@command -v valgrind >/dev/null 2>&1 || { echo "$(RED)❌ Valgrind n'est pas installé. Installez-le avec : sudo pacman -S valgrind$(NC)"; exit 1; }
	@if [ -f $(EXECUTABLE) ]; then \
		valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes $(EXECUTABLE); \
	else \
		echo "$(RED)❌ Erreur : L'exécutable n'existe pas$(NC)"; \
		exit 1; \
	fi

# Débogage avec GDB en mode automatique (lance et affiche la stack trace en cas de crash)
debug-auto: build-debug
	@echo "$(YELLOW)🐞 Lancement en mode automatique...$(NC)"
	@command -v gdb >/dev/null 2>&1 || { echo "$(RED)❌ GDB n'est pas installé. Installez-le avec : sudo pacman -S gdb$(NC)"; exit 1; }
	@if [ -f $(EXECUTABLE) ]; then \
		gdb -batch -ex "run" -ex "bt" -ex "quit" $(EXECUTABLE) 2>&1 || true; \
	else \
		echo "$(RED)❌ Erreur : L'exécutable n'existe pas$(NC)"; \
		exit 1; \
	fi

# Installation
# Installe l'application sur n'importe quel système Linux
install: build
	@printf "$(YELLOW)📦 Installation de l'application...$(NC)\n"
	@if [ -f $(BUILD_DIR)/CMakeCache.txt ]; then \
		cd $(BUILD_DIR) && sudo $(MAKE) install; \
	else \
		printf "$(RED)❌ Erreur : Le projet n'a pas été configuré. Lancez 'make build' d'abord.$(NC)\n"; \
		exit 1; \
	fi
	@printf "$(GREEN)✔️  Application installée avec succès dans $(INSTALL_DIR)$(NC)\n"
	@printf "$(YELLOW)💡 Vous pouvez maintenant lancer l'application avec : $(INSTALL_DIR)/bin/$(PROJECT_NAME)$(NC)\n"

# Réinstallation (désinstalle puis réinstalle)
reinstall: uninstall install

# Désinstallation
uninstall:
	@printf "$(YELLOW)🗑️  Désinstallation de l'application...$(NC)\n"
	@if [ -f $(BUILD_DIR)/CMakeCache.txt ]; then \
		cd $(BUILD_DIR) && sudo $(MAKE) uninstall 2>/dev/null || \
		(sudo rm -f $(INSTALL_DIR)/bin/$(PROJECT_NAME) && \
		 sudo rm -rf $(INSTALL_DIR)/share/WeedlyWeb && \
		 printf "$(GREEN)✔️  Application désinstallée$(NC)\n"); \
	else \
		# Désinstallation manuelle si CMake n'est pas configuré
		sudo rm -f $(INSTALL_DIR)/bin/$(PROJECT_NAME) 2>/dev/null || true; \
		sudo rm -rf $(INSTALL_DIR)/share/WeedlyWeb 2>/dev/null || true; \
		printf "$(GREEN)✔️  Application désinstallée$(NC)\n"; \
	fi

# Monitoring du processus
monitor:
	@echo "$(YELLOW)📈 Surveillance du processus $(PROJECT_NAME)...$(NC)"
	@while true; do \
		clear; \
		PID=$$(pgrep -f "$(EXECUTABLE)"); \
		if [ -z "$$PID" ]; then \
			echo "$(YELLOW)⚠️  Aucun processus détecté. En attente...$(NC)"; \
		else \
			echo "$(GREEN)✅ Processus détecté : PID = $$PID$(NC)"; \
			ps -p $$PID -o pid,%cpu,%mem,cmd; \
		fi; \
		sleep 2; \
	done

# Tests (à implémenter si nécessaire)
test: build
	@echo "$(YELLOW)🧪 Lancement des tests...$(NC)"
	@echo "$(YELLOW)⚠️  Aucun test configuré pour le moment$(NC)"

# Installation des dépendances
install-deps: install-dependencies

install-dependencies:
	@echo "$(YELLOW)📦 Installation des dépendances pour $(PROJECT_NAME)...$(NC)"
	@echo ""
	@echo "$(YELLOW)🔍 Détection de la distribution...$(NC)"
	@if [ -f /etc/arch-release ] || [ -f /etc/manjaro-release ]; then \
		echo "$(GREEN)✅ Distribution détectée : Arch/Manjaro$(NC)"; \
		echo "$(YELLOW)📦 Installation des dépendances GTK...$(NC)"; \
		sudo pacman -S --needed cmake gtk3 webkit2gtk sqlite curl pkg-config gdb valgrind strace || true; \
		echo "$(YELLOW)📦 Installation des dépendances Qt6...$(NC)"; \
		sudo pacman -S --needed qt6-base qt6-webengine || true; \
		echo "$(GREEN)✅ Dépendances installées$(NC)"; \
	elif [ -f /etc/debian_version ]; then \
		echo "$(GREEN)✅ Distribution détectée : Debian/Ubuntu$(NC)"; \
		echo "$(YELLOW)📦 Installation des dépendances GTK...$(NC)"; \
		sudo apt-get update && sudo apt-get install -y cmake libgtk-3-dev libwebkit2gtk-4.1-dev libsqlite3-dev libcurl4-openssl-dev pkg-config gdb valgrind strace || true; \
		echo "$(YELLOW)📦 Installation des dépendances Qt6...$(NC)"; \
		sudo apt-get install -y qt6-base-dev qt6-webengine-dev || true; \
		echo "$(GREEN)✅ Dépendances installées$(NC)"; \
	elif [ -f /etc/fedora-release ]; then \
		echo "$(GREEN)✅ Distribution détectée : Fedora$(NC)"; \
		echo "$(YELLOW)📦 Installation des dépendances GTK...$(NC)"; \
		sudo dnf install -y cmake gtk3-devel webkit2gtk3-devel sqlite-devel libcurl-devel pkg-config gdb valgrind strace || true; \
		echo "$(YELLOW)📦 Installation des dépendances Qt6...$(NC)"; \
		sudo dnf install -y qt6-qtbase-devel qt6-qtwebengine-devel || true; \
		echo "$(GREEN)✅ Dépendances installées$(NC)"; \
	else \
		echo "$(YELLOW)⚠️  Distribution non reconnue$(NC)"; \
		echo "$(YELLOW)💡 Installation manuelle requise$(NC)"; \
		echo "$(YELLOW)📦 Dépendances GTK : cmake, gtk3, webkit2gtk, sqlite, curl, pkg-config$(NC)"; \
		echo "$(YELLOW)📦 Dépendances Qt6 : qt6-base, qt6-webengine$(NC)"; \
		echo "$(YELLOW)💡 Voir docs/INSTALL_DEPENDENCIES.md pour plus d'informations$(NC)"; \
	fi

# Configuration complète : installation des dépendances + compilation
setup: install-deps build
	@echo "$(GREEN)✅ Configuration complète terminée !$(NC)"
	@echo "$(YELLOW)💡 Vous pouvez maintenant lancer l'application avec : make run$(NC)"

# Vérification des dépendances
check-deps:
	@echo "$(YELLOW)🔍 Vérification des dépendances...$(NC)"
	@echo ""
	@echo "$(YELLOW)📦 Dépendances communes obligatoires :$(NC)"
	@command -v $(CMAKE) >/dev/null 2>&1 && echo "$(GREEN)✅ CMake$(NC)" || { echo "$(RED)❌ CMake n'est pas installé$(NC)"; exit 1; }
	@pkg-config --exists sqlite3 && echo "$(GREEN)✅ SQLite3$(NC)" || { echo "$(RED)❌ SQLite3 n'est pas installé$(NC)"; exit 1; }
	@command -v curl-config >/dev/null 2>&1 && echo "$(GREEN)✅ cURL$(NC)" || { echo "$(RED)❌ cURL n'est pas installé$(NC)"; exit 1; }
	@command -v pkg-config >/dev/null 2>&1 && echo "$(GREEN)✅ pkg-config$(NC)" || { echo "$(RED)❌ pkg-config n'est pas installé$(NC)"; exit 1; }
	@echo ""
	@echo "$(YELLOW)📦 Dépendances GTK (pour make build) :$(NC)"
	@pkg-config --exists webkit2gtk-4.1 && echo "$(GREEN)✅ WebKit2GTK 4.1$(NC)" || echo "$(YELLOW)⚠️  WebKit2GTK 4.1 n'est pas installé (optionnel si Qt utilisé)$(NC)"
	@pkg-config --exists gtk+-3.0 && echo "$(GREEN)✅ GTK+3$(NC)" || echo "$(YELLOW)⚠️  GTK+3 n'est pas installé (optionnel si Qt utilisé)$(NC)"
	@echo ""
	@echo "$(YELLOW)📦 Dépendances Qt6 (pour make build-qt) :$(NC)"
	@pkg-config --exists Qt6Core Qt6Gui Qt6Widgets Qt6WebEngineWidgets && echo "$(GREEN)✅ Qt6 (Core, Gui, Widgets, WebEngine)$(NC)" || echo "$(YELLOW)⚠️  Qt6 n'est pas installé (optionnel si GTK utilisé)$(NC)"
	@echo ""
	@echo "$(YELLOW)🐞 Outils de débogage (optionnels mais recommandés) :$(NC)"
	@command -v gdb >/dev/null 2>&1 && echo "$(GREEN)✅ GDB$(NC)" || echo "$(YELLOW)⚠️  GDB n'est pas installé (optionnel)$(NC)"
	@command -v valgrind >/dev/null 2>&1 && echo "$(GREEN)✅ Valgrind$(NC)" || echo "$(YELLOW)⚠️  Valgrind n'est pas installé (optionnel)$(NC)"
	@command -v strace >/dev/null 2>&1 && echo "$(GREEN)✅ strace$(NC)" || echo "$(YELLOW)⚠️  strace n'est pas installé (optionnel)$(NC)"
	@echo ""
	@if pkg-config --exists Qt6Core Qt6Gui Qt6Widgets Qt6WebEngineWidgets 2>/dev/null; then \
		echo "$(GREEN)✔️  Qt6 disponible - vous pouvez utiliser 'make build-qt'$(NC)"; \
	elif pkg-config --exists webkit2gtk-4.1 gtk+-3.0 2>/dev/null; then \
		echo "$(GREEN)✔️  GTK disponible - vous pouvez utiliser 'make build'$(NC)"; \
	else \
		echo "$(RED)❌ Aucune dépendance UI trouvée$(NC)"; \
		echo "$(YELLOW)💡 Installez Qt6 : sudo pacman -S qt6-base qt6-webengine$(NC)"; \
		echo "$(YELLOW)💡 OU installez GTK : sudo pacman -S gtk3 webkit2gtk$(NC)"; \
	fi

# Informations sur le projet
info:
	@echo "$(GREEN)📋 Informations sur $(PROJECT_NAME)$(NC)"
	@echo ""
	@echo "$(YELLOW)Version :$(NC) 1.0"
	@echo "$(YELLOW)Build dir :$(NC) $(BUILD_DIR)"
	@echo "$(YELLOW)Exécutable :$(NC) $(EXECUTABLE)"
	@echo "$(YELLOW)Install dir :$(NC) $(INSTALL_DIR)"
	@echo ""
	@if [ -f $(EXECUTABLE) ]; then \
		echo "$(GREEN)✅ Exécutable trouvé$(NC)"; \
		ls -lh $(EXECUTABLE); \
	else \
		echo "$(YELLOW)⚠️  Exécutable non trouvé (compiler avec 'make build')$(NC)"; \
	fi

# Mode watch : surveille les fichiers et recompile automatiquement
watch:
	@echo "$(YELLOW)👀 Mode watch activé - Surveillance des fichiers source...$(NC)"
	@echo "$(YELLOW)💡 Appuyez sur Ctrl+C pour arrêter$(NC)"
	@echo ""
	@if command -v inotifywait >/dev/null 2>&1; then \
		echo "$(GREEN)✅ Utilisation de inotifywait$(NC)"; \
		while true; do \
			inotifywait -r -e modify,create,delete,move --include='\.(cpp|h|hpp|cmake|CMakeLists\.txt)$$' \
				--exclude='$(BUILD_DIR)' \
				$(SOURCE_DIR)/src $(SOURCE_DIR)/include $(SOURCE_DIR)/CMakeLists.txt 2>/dev/null && \
			echo "$(YELLOW)📝 Fichier modifié, recompilation...$(NC)" && \
			$(MAKE) build || true; \
		done; \
	elif command -v entr >/dev/null 2>&1; then \
		echo "$(GREEN)✅ Utilisation de entr$(NC)"; \
		find $(SOURCE_DIR)/src $(SOURCE_DIR)/include -name "*.cpp" -o -name "*.h" -o -name "*.hpp" | \
		entr -p $(MAKE) build; \
	else \
		echo "$(RED)❌ Aucun outil de surveillance trouvé (inotifywait ou entr)$(NC)"; \
		echo "$(YELLOW)💡 Installation : sudo pacman -S inotify-tools$(NC)"; \
		echo "$(YELLOW)🔄 Utilisation d'un mode de surveillance basique (polling)...$(NC)"; \
		$(MAKE) watch-basic; \
	fi

# Mode watch basique (polling) si inotifywait/entr ne sont pas disponibles
watch-basic:
	@echo "$(YELLOW)👀 Mode watch basique (polling toutes les 2 secondes)...$(NC)"
	@echo "$(YELLOW)💡 Appuyez sur Ctrl+C pour arrêter$(NC)"
	@echo "$(YELLOW)💡 Pour un meilleur mode watch, installez: sudo pacman -S inotify-tools$(NC)"
	@echo ""
	@LAST_BUILD=$$(find $(SOURCE_DIR)/src $(SOURCE_DIR)/include -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) -exec stat -c %Y {} \; | sort -n | tail -1); \
	while true; do \
		CURRENT_BUILD=$$(find $(SOURCE_DIR)/src $(SOURCE_DIR)/include -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) -exec stat -c %Y {} \; | sort -n | tail -1); \
		if [ "$$CURRENT_BUILD" != "$$LAST_BUILD" ]; then \
			echo "$(YELLOW)📝 Fichier modifié, recompilation...$(NC)"; \
			$(MAKE) build || true; \
			LAST_BUILD=$$CURRENT_BUILD; \
		fi; \
		sleep 2; \
	done

# Mode watch avec relance automatique de l'application
watch-run:
	@echo "$(YELLOW)👀 Mode watch avec relance automatique activé...$(NC)"
	@echo "$(YELLOW)💡 Appuyez sur Ctrl+C pour arrêter$(NC)"
	@echo ""
	@PID_FILE=/tmp/weedlyweb-watch.pid; \
	trap 'kill $$(cat $$PID_FILE 2>/dev/null) 2>/dev/null; rm -f $$PID_FILE; exit' INT TERM; \
	if command -v inotifywait >/dev/null 2>&1; then \
		echo "$(GREEN)✅ Utilisation de inotifywait$(NC)"; \
		$(MAKE) build && $(MAKE) run-bg; \
		echo $$! > $$PID_FILE; \
		while true; do \
			inotifywait -r -e modify,create,delete,move --include='\.(cpp|h|hpp|cmake|CMakeLists\.txt)$$' \
				--exclude='$(BUILD_DIR)' \
				$(SOURCE_DIR)/src $(SOURCE_DIR)/include $(SOURCE_DIR)/CMakeLists.txt 2>/dev/null && \
			echo "$(YELLOW)📝 Fichier modifié, recompilation...$(NC)" && \
			kill $$(cat $$PID_FILE 2>/dev/null) 2>/dev/null; \
			$(MAKE) build && \
			echo "$(GREEN)✅ Recompilation terminée, relance de l'application...$(NC)" && \
			$(MAKE) run-bg; \
			echo $$! > $$PID_FILE; \
		done; \
	elif command -v entr >/dev/null 2>&1; then \
		echo "$(GREEN)✅ Utilisation de entr$(NC)"; \
		$(MAKE) build && $(MAKE) run-bg; \
		echo $$! > $$PID_FILE; \
		find $(SOURCE_DIR)/src $(SOURCE_DIR)/include -name "*.cpp" -o -name "*.h" -o -name "*.hpp" | \
		entr -p sh -c 'kill $$(cat /tmp/weedlyweb-watch.pid 2>/dev/null) 2>/dev/null; make build && make run-bg; echo $$! > /tmp/weedlyweb-watch.pid'; \
	else \
		echo "$(RED)❌ Aucun outil de surveillance trouvé (inotifywait ou entr)$(NC)"; \
		echo "$(YELLOW)💡 Installation : sudo pacman -S inotify-tools$(NC)"; \
		echo "$(YELLOW)🔄 Utilisation d'un mode de surveillance basique (polling)...$(NC)"; \
		$(MAKE) watch-run-basic; \
	fi

# Mode watch-run basique (polling)
watch-run-basic:
	@echo "$(YELLOW)👀 Mode watch-run basique (polling toutes les 2 secondes)...$(NC)"
	@echo "$(YELLOW)💡 Appuyez sur Ctrl+C pour arrêter$(NC)"
	@echo "$(YELLOW)💡 Pour un meilleur mode watch, installez: sudo pacman -S inotify-tools$(NC)"
	@echo ""
	@PID_FILE=/tmp/weedlyweb-watch.pid; \
	trap 'kill $$(cat $$PID_FILE 2>/dev/null) 2>/dev/null; rm -f $$PID_FILE; exit' INT TERM; \
	$(MAKE) build && $(MAKE) run-bg; \
	echo $$! > $$PID_FILE; \
	LAST_BUILD=$$(find $(SOURCE_DIR)/src $(SOURCE_DIR)/include -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) -exec stat -c %Y {} \; | sort -n | tail -1); \
	while true; do \
		CURRENT_BUILD=$$(find $(SOURCE_DIR)/src $(SOURCE_DIR)/include -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) -exec stat -c %Y {} \; | sort -n | tail -1); \
		if [ "$$CURRENT_BUILD" != "$$LAST_BUILD" ]; then \
			echo "$(YELLOW)📝 Fichier modifié, recompilation...$(NC)"; \
			kill $$(cat $$PID_FILE 2>/dev/null) 2>/dev/null; \
			$(MAKE) build && \
			echo "$(GREEN)✅ Recompilation terminée, relance de l'application...$(NC)" && \
			$(MAKE) run-bg; \
			echo $$! > $$PID_FILE; \
			LAST_BUILD=$$CURRENT_BUILD; \
		fi; \
		sleep 2; \
	done


# Mode développement : surveille, recompile et recharge proprement l'application
dev:
	@PID_FILE=/tmp/weedlyweb-dev.pid; \
	WATCH_PID=; \
	cleanup() { \
		echo ""; \
		echo "$(YELLOW)🛑 Arrêt en cours...$(NC)"; \
		if [ -n "$$WATCH_PID" ] && kill -0 $$WATCH_PID 2>/dev/null; then \
			kill -TERM $$WATCH_PID 2>/dev/null; \
			kill -KILL $$WATCH_PID 2>/dev/null || true; \
		fi; \
		if [ -f $$PID_FILE ]; then \
			PID=$$(cat $$PID_FILE 2>/dev/null); \
			if [ -n "$$PID" ] && kill -0 $$PID 2>/dev/null; then \
				echo "$(YELLOW)🛑 Arrêt propre de l'application (PID: $$PID)...$(NC)"; \
				kill -TERM $$PID 2>/dev/null; \
				sleep 0.5; \
				if kill -0 $$PID 2>/dev/null; then \
					kill -KILL $$PID 2>/dev/null || true; \
				fi; \
			fi; \
			rm -f $$PID_FILE; \
		fi; \
		pkill -f "$(EXECUTABLE)" 2>/dev/null || true; \
		echo "$(GREEN)✅ Arrêt terminé$(NC)"; \
	}; \
	trap 'cleanup; exit 0' INT TERM EXIT; \
	echo "$(GREEN)🚀 Mode développement activé$(NC)"; \
	echo "$(YELLOW)💡 L'application sera automatiquement recompilée et rechargée à chaque modification$(NC)"; \
	echo "$(YELLOW)💡 Appuyez sur Ctrl+C pour arrêter proprement$(NC)"; \
	echo ""; \
	launch_app() { \
		if [ -f $$PID_FILE ]; then \
			PID=$$(cat $$PID_FILE 2>/dev/null); \
			if [ -n "$$PID" ] && kill -0 $$PID 2>/dev/null; then \
				echo "$(YELLOW)🛑 Arrêt de l'ancienne instance...$(NC)"; \
				kill -TERM $$PID 2>/dev/null; \
				sleep 0.3; \
				kill -KILL $$PID 2>/dev/null || true; \
				rm -f $$PID_FILE; \
			fi; \
		fi; \
		echo "$(YELLOW)🔨 Compilation...$(NC)"; \
		if $(MAKE) build >/dev/null 2>&1; then \
			echo "$(GREEN)✅ Compilation réussie$(NC)"; \
			echo "$(YELLOW)🚀 Lancement de l'application...$(NC)"; \
			$(EXECUTABLE) >/dev/null 2>&1 & \
			APP_PID=$$!; \
			echo $$APP_PID > $$PID_FILE; \
			sleep 0.5; \
			if kill -0 $$APP_PID 2>/dev/null; then \
				echo "$(GREEN)✅ Application lancée (PID: $$APP_PID)$(NC)"; \
			else \
				echo "$(RED)❌ L'application n'a pas pu démarrer$(NC)"; \
				rm -f $$PID_FILE; \
			fi; \
		else \
			echo "$(RED)❌ Erreur de compilation$(NC)"; \
		fi; \
	}; \
	launch_app; \
	if command -v inotifywait >/dev/null 2>&1; then \
		echo "$(GREEN)✅ Utilisation de inotifywait (surveillance en temps réel)$(NC)"; \
		(inotifywait -r -m -q -e modify,create,delete,move --include='\.(cpp|h|hpp|cmake|CMakeLists\.txt)$$' \
			--exclude='$(BUILD_DIR)' \
			$(SOURCE_DIR)/src $(SOURCE_DIR)/include $(SOURCE_DIR)/CMakeLists.txt 2>/dev/null | \
		while read -r event; do \
			echo ""; \
			echo "$(YELLOW)📝 Modification détectée...$(NC)"; \
			launch_app; \
		done) & \
		WATCH_PID=$$!; \
		wait $$WATCH_PID; \
	elif command -v entr >/dev/null 2>&1; then \
		echo "$(GREEN)✅ Utilisation de entr (surveillance en temps réel)$(NC)"; \
		(find $(SOURCE_DIR)/src $(SOURCE_DIR)/include $(SOURCE_DIR)/CMakeLists.txt -name "*.cpp" -o -name "*.h" -o -name "*.hpp" -o -name "CMakeLists.txt" | \
		entr -p sh -c 'echo ""; echo "$(YELLOW)📝 Modification détectée...$(NC)"; \
			PID_FILE=/tmp/weedlyweb-dev.pid; \
			if [ -f $$PID_FILE ]; then \
				PID=$$(cat $$PID_FILE 2>/dev/null); \
				if [ -n "$$PID" ] && kill -0 $$PID 2>/dev/null; then \
					kill -TERM $$PID 2>/dev/null; \
					sleep 0.3; \
					kill -KILL $$PID 2>/dev/null || true; \
				fi; \
			fi; \
			make build >/dev/null 2>&1 && \
			$(EXECUTABLE) >/dev/null 2>&1 & \
			echo $$! > $$PID_FILE; \
			sleep 0.5; \
			if kill -0 $$(cat $$PID_FILE) 2>/dev/null; then \
				echo "$(GREEN)✅ Application rechargée$(NC)"; \
			fi') & \
		WATCH_PID=$$!; \
		wait $$WATCH_PID; \
	else \
		echo "$(YELLOW)⚠️  inotifywait/entr non trouvés, utilisation du mode polling$(NC)"; \
		echo "$(YELLOW)💡 Pour une meilleure expérience : sudo pacman -S inotify-tools$(NC)"; \
		echo ""; \
		LAST_BUILD=$$(find $(SOURCE_DIR)/src $(SOURCE_DIR)/include -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) -exec stat -c %Y {} \; | sort -n | tail -1); \
		while true; do \
			CURRENT_BUILD=$$(find $(SOURCE_DIR)/src $(SOURCE_DIR)/include -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) -exec stat -c %Y {} \; | sort -n | tail -1); \
			if [ "$$CURRENT_BUILD" != "$$LAST_BUILD" ]; then \
				echo ""; \
				echo "$(YELLOW)📝 Modification détectée...$(NC)"; \
				launch_app; \
				LAST_BUILD=$$CURRENT_BUILD; \
			fi; \
			sleep 1; \
		done; \
	fi
