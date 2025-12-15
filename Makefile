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

.PHONY: all clean build run debug install help monitor test configure

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
	@echo "  $(GREEN)make run$(NC)           - Compile et lance l'application"
	@echo "  $(GREEN)make debug$(NC)        - Compile et lance avec GDB"
	@echo "  $(GREEN)make install$(NC)      - Installe l'application"
	@echo "  $(GREEN)make monitor$(NC)       - Surveille le processus en cours"
	@echo "  $(GREEN)make test$(NC)         - Lance les tests (si disponibles)"
	@echo "  $(GREEN)make configure$(NC)   - Configure CMake uniquement"
	@echo "  $(GREEN)make rebuild$(NC)      - Nettoie et recompile"
	@echo ""

# Configuration CMake
configure:
	@echo "$(YELLOW)⚙️  Configuration CMake...$(NC)"
	@mkdir -p $(BUILD_DIR)
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
	@rm -f $(SOURCE_DIR)/Makefile
	@rm -f $(SOURCE_DIR)/cmake_install.cmake
	@echo "$(GREEN)✔️  Nettoyage terminé$(NC)"

# Rebuild complet
rebuild: clean build

# Exécution
run: build
	@echo "$(GREEN)🚀 Lancement de $(PROJECT_NAME)...$(NC)"
	@if [ -f $(EXECUTABLE) ]; then \
		$(EXECUTABLE) & \
		echo "$(GREEN)✅ $(PROJECT_NAME) lancé avec le PID : $$!$(NC)"; \
	else \
		echo "$(RED)❌ Erreur : L'exécutable n'existe pas$(NC)"; \
		exit 1; \
	fi

# Débogage avec GDB
debug: build
	@echo "$(YELLOW)🐞 Lancement en mode debug avec GDB...$(NC)"
	@if [ -f $(EXECUTABLE) ]; then \
		gdb $(EXECUTABLE); \
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
	@command -v $(CMAKE) >/dev/null 2>&1 || { echo "$(RED)❌ CMake n'est pas installé$(NC)"; exit 1; }
	@pkg-config --exists webkit2gtk-4.1 || { echo "$(RED)❌ WebKit2GTK 4.1 n'est pas installé$(NC)"; exit 1; }
	@pkg-config --exists gtk+-3.0 || { echo "$(RED)❌ GTK+3 n'est pas installé$(NC)"; exit 1; }
	@command -v curl-config >/dev/null 2>&1 || { echo "$(RED)❌ cURL n'est pas installé$(NC)"; exit 1; }
	@echo "$(GREEN)✔️  Toutes les dépendances sont installées$(NC)"

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

