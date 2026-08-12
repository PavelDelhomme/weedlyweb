# Makefile wrapper pour WeedlyWeb
# Ce fichier intercepte "make help" et délègue les autres cibles au Makefile généré par CMake

.PHONY: help project-help run run-debug run-gtk run-qt analyze-memory valgrind stop

# Cible help personnalisée
help:
	@echo ""
	@echo "🌐 WeedlyWeb - Guide de démarrage"
	@echo "=================================="
	@echo ""
	@echo "📋 Commandes principales :"
	@echo ""
	@echo "  make              Compile le projet (GTK + Qt si dispo)"
	@echo "  make run          Compile et lance GTK (multi-backend display)"
	@echo "  make run-gtk      Lance le backend GTK"
	@echo "  make run-qt       Lance le backend Qt6"
	@echo "  make run-debug    Compile Debug + lance GTK"
	@echo "  make clean        Nettoie le répertoire de build"
	@echo "  make install      Installe l'application"
	@echo ""
	@echo "📂 Structure UI :"
	@echo "  src/core/           logique partagée"
	@echo "  src/ui/gtk/         backend GTK/WebKit2GTK"
	@echo "  src/ui/qt/          backend Qt6/WebEngine"
	@echo "  src/ui/{android,cocoa,win32}/  stubs futurs"
	@echo ""
	@echo "📚 docs/COMPATIBILITY.md — X11/Wayland/DE"
	@echo ""

# S'assurer que build/Makefile existe avant de déléguer
build/Makefile:
	@echo "⚠️  Le répertoire de build n'est pas configuré."
	@echo "💡 Configuration de CMake..."
	@cmake -B build -S . -DBUILD_UI_GTK=ON -DBUILD_UI_QT=ON

# Cible run : compile et lance GTK via lanceur multi-backend
run: run-gtk

run-gtk: build/Makefile
	@echo "🔨 Compilation..."
	@cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DBUILD_UI_GTK=ON
	@$(MAKE) -C build WeedlyWeb
	@echo "🚀 Lancement GTK..."
	@chmod +x scripts/run-weedlyweb.sh 2>/dev/null || true
	@WEEDLYWEB_UI=gtk ./scripts/run-weedlyweb.sh

run-qt: build/Makefile
	@echo "🔨 Compilation Qt..."
	@cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DBUILD_UI_QT=ON
	@$(MAKE) -C build WeedlyWebQt
	@echo "🚀 Lancement Qt..."
	@chmod +x scripts/run-weedlyweb.sh 2>/dev/null || true
	@WEEDLYWEB_UI=qt ./scripts/run-weedlyweb.sh

run-debug: build/Makefile
	@echo "🔨 Compilation Debug..."
	@cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug -DBUILD_UI_GTK=ON
	@$(MAKE) -C build WeedlyWeb
	@echo "🐛 Lancement GTK (debug)..."
	@chmod +x scripts/run-weedlyweb.sh 2>/dev/null || true
	@WEEDLYWEB_UI=gtk ./scripts/run-weedlyweb.sh

# Cible pour analyser la mémoire
analyze-memory:
	@if [ -f scripts/analyze-memory.sh ]; then \
		./scripts/analyze-memory.sh; \
	else \
		echo "❌ Script d'analyse mémoire non trouvé"; \
	fi

# Cible pour lancer avec Valgrind
valgrind:
	@if [ -f scripts/run-valgrind.sh ]; then \
		./scripts/run-valgrind.sh; \
	else \
		echo "❌ Script Valgrind non trouvé"; \
		echo "💡 Utilisez: valgrind --leak-check=full --show-leak-kinds=all ./build/WeedlyWeb"; \
	fi

# Cible pour arrêter tous les processus WeedlyWeb
stop:
	@if [ -f scripts/stop-weedlyweb.sh ]; then \
		./scripts/stop-weedlyweb.sh; \
	else \
		echo "❌ Script d'arrêt non trouvé"; \
		echo "💡 Utilisez: pkill -f WeedlyWeb"; \
	fi

# Déléguer toutes les autres cibles au Makefile généré par CMake
# Utiliser une règle générique qui capture toutes les cibles sauf "help", "run", "run-debug"
%: build/Makefile
	@$(MAKE) -C build $@

