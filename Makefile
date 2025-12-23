# Makefile wrapper pour WeedlyWeb
# Ce fichier intercepte "make help" et délègue les autres cibles au Makefile généré par CMake

.PHONY: help project-help run run-debug analyze-memory valgrind stop

# Cible help personnalisée
help:
	@echo ""
	@echo "🌐 WeedlyWeb - Guide de démarrage"
	@echo "=================================="
	@echo ""
	@echo "📋 Commandes principales :"
	@echo ""
	@echo "  make              Compile le projet"
	@echo "  make run          Compile et lance l'application"
	@echo "  make run-debug    Compile en mode debug et lance l'application"
	@echo "  make clean        Nettoie le répertoire de build"
	@echo "  make install      Installe l'application"
	@echo "  make analyze-memory  Analyse l'utilisation mémoire du projet"
	@echo "  make valgrind       Lance l'application avec Valgrind (détection de fuites)"
	@echo "  make stop           Arrête tous les processus WeedlyWeb"
	@echo ""
	@echo "🚀 Pour démarrer rapidement :"
	@echo ""
	@echo "  1. Installer les dépendances :"
	@echo "     ./scripts/install-deps.sh"
	@echo ""
	@echo "  2. Configurer CMake (si pas déjà fait) :"
	@echo "     cmake -B build -S ."
	@echo ""
	@echo "  3. Compiler :"
	@echo "     make"
	@echo ""
	@echo "  4. Lancer l'application :"
	@if [ -f build/WeedlyWeb ]; then \
		echo "     ./build/WeedlyWeb"; \
	elif [ -f build/ui-qt/build/WeedlyWebQt ]; then \
		echo "     ./build/ui-qt/build/WeedlyWebQt"; \
	else \
		echo "     ./build/WeedlyWeb (après compilation)"; \
	fi
	@echo ""
	@echo "📚 Documentation :"
	@echo ""
	@echo "  - README.md                    Documentation principale"
	@echo "  - docs/QUICKSTART.md           Guide de démarrage rapide"
	@echo "  - docs/INSTALL_DEPENDENCIES.md Guide d'installation"
	@echo ""
	@echo "💡 Astuce : Consultez README.md pour plus d'informations !"
	@echo ""
	@echo "=================================="
	@echo ""

# S'assurer que build/Makefile existe avant de déléguer
build/Makefile:
	@echo "⚠️  Le répertoire de build n'est pas configuré."
	@echo "💡 Configuration de CMake..."
	@cmake -B build -S .

# Cible run : compile en mode Release et lance l'application
run: build/Makefile
	@echo "🔨 Compilation en mode Release..."
	@cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
	@$(MAKE) -C build
	@echo ""
	@echo "🚀 Lancement de l'application..."
	@EXECUTABLE=""; \
	if [ -f build/WeedlyWeb ]; then \
		EXECUTABLE="build/WeedlyWeb"; \
	elif [ -f build/ui-qt/build/WeedlyWebQt ]; then \
		EXECUTABLE="build/ui-qt/build/WeedlyWebQt"; \
	fi; \
	if [ -z "$$EXECUTABLE" ]; then \
		echo "❌ Erreur: Exécutable non trouvé après compilation."; \
		echo "💡 Vérifiez que la compilation s'est bien terminée."; \
		exit 1; \
	else \
		echo "▶️  Exécution: ./$$EXECUTABLE"; \
		\
		# Désactiver l'accélération GPU pour éviter les erreurs GBM \
		export WEBKIT_DISABLE_COMPOSITING_MODE=1; \
		export LIBGL_ALWAYS_SOFTWARE=1; \
		\
		# Essayer d'abord avec le backend par défaut \
		./$$EXECUTABLE 2>&1; \
		EXIT_CODE=$$?; \
		\
		# Si erreur, essayer avec X11 \
		if [ $$EXIT_CODE -ne 0 ]; then \
			echo ""; \
			echo "⚠️  Erreur détectée (code: $$EXIT_CODE), tentative avec X11..."; \
			echo "🖥️  Lancement avec backend X11 (GDK_BACKEND=x11)..."; \
			GDK_BACKEND=x11 ./$$EXECUTABLE 2>&1; \
			X11_EXIT=$$?; \
			\
			if [ $$X11_EXIT -eq 0 ]; then \
				echo "✅ Application terminée normalement avec X11."; \
			else \
				echo ""; \
				echo "❌ Erreur lors de l'exécution avec X11 (code: $$X11_EXIT)"; \
				echo "💡 Vérifiez que X11 est disponible: echo $$DISPLAY"; \
				exit $$X11_EXIT; \
			fi; \
		fi; \
	fi

# Cible run-debug : compile en mode Debug et lance l'application
run-debug: build/Makefile
	@echo "🔨 Compilation en mode Debug..."
	@cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
	@$(MAKE) -C build
	@echo ""
	@echo "🐛 Lancement de l'application en mode debug..."
	@EXECUTABLE=""; \
	if [ -f build/WeedlyWeb ]; then \
		EXECUTABLE="build/WeedlyWeb"; \
	elif [ -f build/ui-qt/build/WeedlyWebQt ]; then \
		EXECUTABLE="build/ui-qt/build/WeedlyWebQt"; \
	fi; \
	if [ -z "$$EXECUTABLE" ]; then \
		echo "❌ Erreur: Exécutable non trouvé après compilation."; \
		echo "💡 Vérifiez que la compilation s'est bien terminée."; \
		exit 1; \
	else \
		echo "▶️  Exécution: ./$$EXECUTABLE"; \
		\
		# Désactiver l'accélération GPU pour éviter les erreurs GBM \
		export WEBKIT_DISABLE_COMPOSITING_MODE=1; \
		export LIBGL_ALWAYS_SOFTWARE=1; \
		\
		# Essayer d'abord avec le backend par défaut \
		./$$EXECUTABLE 2>&1; \
		EXIT_CODE=$$?; \
		\
		# Si erreur, essayer avec X11 \
		if [ $$EXIT_CODE -ne 0 ]; then \
			echo ""; \
			echo "⚠️  Erreur détectée (code: $$EXIT_CODE), tentative avec X11..."; \
			echo "🖥️  Lancement avec backend X11 (GDK_BACKEND=x11)..."; \
			GDK_BACKEND=x11 ./$$EXECUTABLE 2>&1; \
			X11_EXIT=$$?; \
			\
			if [ $$X11_EXIT -eq 0 ]; then \
				echo "✅ Application terminée normalement avec X11."; \
			else \
				echo ""; \
				echo "❌ Erreur lors de l'exécution avec X11 (code: $$X11_EXIT)"; \
				echo "💡 Vérifiez que X11 est disponible: echo $$DISPLAY"; \
				exit $$X11_EXIT; \
			fi; \
		fi; \
	fi

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

