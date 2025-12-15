#!/bin/bash
# pour créer la structure monorepo Script

set -e

echo "🏗️  Création de la structure monorepo..."

# Créer les dossiers core
mkdir -p core/include/{browser,database,http,storage,memory,favorites,tabs,utils}
mkdir -p core/src/{browser,database,http,storage,memory,favorites,tabs,utils}

# Créer les dossiers ui-gtk
mkdir -p ui-gtk/include/gtk
mkdir -p ui-gtk/src/gtk

# Créer les dossiers ui-qt
mkdir -p ui-qt/include/qt
mkdir -p ui-qt/src/qt

# Créer les dossiers ui-android
mkdir -p ui-android/{app,jni}

# Créer les dossiers de tests
mkdir -p tests/{core,ui-gtk}

# Créer les dossiers de documentation
mkdir -p docs/{core,ui-gtk,ui-qt,ui-android}

echo "✅ Structure créée avec succès !"
echo ""
echo "📁 Structure créée :"
echo "  - core/          : Bibliothèque C++ indépendante"
echo "  - ui-gtk/        : Application GTK"
echo "  - ui-qt/         : Application Qt (futur)"
echo "  - ui-android/    : Application Android (futur)"
echo "  - tests/         : Tests unitaires"
echo "  - docs/          : Documentation"

