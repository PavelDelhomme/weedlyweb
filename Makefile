# Makefile pour WeedlyWeb
# Navigateur web en C++ avec WebKit2GTK

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

.PHONY: all clean build run run-bg run-debug build-debug debug debug-auto valgrind install help monitor test configure

# Cible par défaut
all: build

# Afficher l'aide
help:
	@echo "$(GREEN)🔧 Makefile pour $(PROJECT_NAME)$(NC)"
	@echo ""
	@echo "$(YELLOW)Commandes disponibles :$(NC)"
	@echo "  $(GREEN)make$(NC)              - Compile le projet (alias de 'make build')"
	@echo "  $(GREEN)make build$(NC)        - Compile le projet"
	@echo "  $(GREEN)make clean$(NC)        - Nettoie le répertoire de build"
	@echo "  $(GREEN)make run$(NC)           - Compile et lance l'application (foreground)"
	@echo "  $(GREEN)make run-bg$(NC)       - Compile et lance en arrière-plan"
	@echo "  $(GREEN)make run-debug$(NC)    - Compile en debug et lance (avec symboles)"
	@echo "  $(GREEN)make debug$(NC)        - Compile en debug et lance avec GDB (interactif)"
	@echo "  $(GREEN)make debug-auto$(NC)   - Compile en debug et lance avec GDB (automatique)"
	@echo "  $(GREEN)make valgrind$(NC)     - Lance avec Valgrind (détection fuites)"
	@echo "  $(GREEN)make install$(NC)      - Installe l'application"
	@echo "  $(GREEN)make monitor$(NC)       - Surveille le processus en cours"
	@echo "  $(GREEN)make test$(NC)         - Lance les tests (si disponibles)"
	@echo "  $(GREEN)make configure$(NC)   - Configure CMake uniquement"
	@echo "  $(GREEN)make rebuild$(NC)      - Nettoie et recompile"
	@echo "  $(GREEN)make check-deps$(NC)   - Vérifie les dépendances installées"
	@echo ""
	@echo "$(YELLOW)💡 Pour installer les dépendances :$(NC)"
	@echo "  $(GREEN)./install-deps.sh$(NC) - Script d'installation automatique"
	@echo "  ou consultez $(GREEN)INSTALL_DEPENDENCIES.md$(NC)"
	@echo ""

# Configuration CMake
configure:
	@echo "$(YELLOW)⚙️  Configuration CMake...$(NC)"
	@mkdir -p $(BUILD_DIR)
	@# Nettoyer le cache CMake si le répertoire source a changé
	@if [ -f $(BUILD_DIR)/CMakeCache.txt ]; then \
		CACHED_SOURCE=$$(grep "^CMAKE_HOME_DIRECTORY:" $(BUILD_DIR)/CMakeCache.txt 2>/dev/null | cut -d= -f2 | tr -d '\n'); \
		CURRENT_SOURCE=$$(pwd); \
		if [ "$$CACHED_SOURCE" != "$$CURRENT_SOURCE" ] && [ -n "$$CACHED_SOURCE" ]; then \
			echo "$(YELLOW)⚠️  Cache CMake détecté depuis un autre répertoire, nettoyage...$(NC)"; \
			rm -rf $(BUILD_DIR)/CMakeCache.txt $(BUILD_DIR)/CMakeFiles; \
		fi; \
	fi
	@cd $(BUILD_DIR) && $(CMAKE) ..
	@echo "$(GREEN)✔️  Configuration terminée$(NC)"

# Compilation
build: configure
	@echo "$(YELLOW)🔨 Compilation du projet...$(NC)"
	@cd $(BUILD_DIR) && $(MAKE) -j$(NPROC)
	@echo "$(GREEN)✔️  Compilation terminée avec succès$(NC)"

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
run: build
	@echo "$(GREEN)🚀 Lancement de $(PROJECT_NAME)...$(NC)"
	@if [ -f $(EXECUTABLE) ]; then \
		$(EXECUTABLE); \
	else \
		echo "$(RED)❌ Erreur : L'exécutable n'existe pas$(NC)"; \
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
	@if [ -f $(EXECUTABLE) ]; then \
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
install: build
	@echo "$(YELLOW)📦 Installation de l'application...$(NC)"
	@cd $(BUILD_DIR) && sudo $(MAKE) install
	@echo "$(GREEN)✔️  Application installée avec succès$(NC)"

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

# Vérification des dépendances
check-deps:
	@echo "$(YELLOW)🔍 Vérification des dépendances...$(NC)"
	@echo ""
	@echo "$(YELLOW)📦 Dépendances obligatoires :$(NC)"
	@command -v $(CMAKE) >/dev/null 2>&1 && echo "$(GREEN)✅ CMake$(NC)" || { echo "$(RED)❌ CMake n'est pas installé$(NC)"; exit 1; }
	@pkg-config --exists webkit2gtk-4.1 && echo "$(GREEN)✅ WebKit2GTK 4.1$(NC)" || { echo "$(RED)❌ WebKit2GTK 4.1 n'est pas installé$(NC)"; exit 1; }
	@pkg-config --exists gtk+-3.0 && echo "$(GREEN)✅ GTK+3$(NC)" || { echo "$(RED)❌ GTK+3 n'est pas installé$(NC)"; exit 1; }
	@pkg-config --exists sqlite3 && echo "$(GREEN)✅ SQLite3$(NC)" || { echo "$(RED)❌ SQLite3 n'est pas installé$(NC)"; exit 1; }
	@command -v curl-config >/dev/null 2>&1 && echo "$(GREEN)✅ cURL$(NC)" || { echo "$(RED)❌ cURL n'est pas installé$(NC)"; exit 1; }
	@command -v pkg-config >/dev/null 2>&1 && echo "$(GREEN)✅ pkg-config$(NC)" || { echo "$(RED)❌ pkg-config n'est pas installé$(NC)"; exit 1; }
	@echo ""
	@echo "$(YELLOW)🐞 Outils de débogage (optionnels mais recommandés) :$(NC)"
	@command -v gdb >/dev/null 2>&1 && echo "$(GREEN)✅ GDB$(NC)" || echo "$(YELLOW)⚠️  GDB n'est pas installé (optionnel)$(NC)"
	@command -v valgrind >/dev/null 2>&1 && echo "$(GREEN)✅ Valgrind$(NC)" || echo "$(YELLOW)⚠️  Valgrind n'est pas installé (optionnel)$(NC)"
	@command -v strace >/dev/null 2>&1 && echo "$(GREEN)✅ strace$(NC)" || echo "$(YELLOW)⚠️  strace n'est pas installé (optionnel)$(NC)"
	@echo ""
	@echo "$(GREEN)✔️  Toutes les dépendances obligatoires sont installées$(NC)"
	@echo "$(YELLOW)💡 Pour installer les outils de débogage : sudo pacman -S gdb valgrind strace$(NC)"

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

