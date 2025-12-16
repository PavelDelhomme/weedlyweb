#!/bin/bash

# Script pour créer la branche hotfix et sauvegarder les modifications

set -e  # Arrêter en cas d'erreur

echo "🔍 Étape 1: Vérification du commit actuel..."
git log --oneline -1
echo ""

echo "🌿 Étape 2: Création de la branche hotfix/err-failed-gpu..."
git checkout -b hotfix/err-failed-gpu
echo "✅ Branche créée"
echo ""

echo "📦 Étape 3: Ajout des fichiers modifiés..."
git add CMakeLists.txt Makefile ui-qt/src/qt/main.cpp .gitignore
echo "✅ Fichiers ajoutés"
echo ""

echo "🔍 Étape 4: Vérification de ce qui sera commité..."
git status
echo ""

echo "💾 Étape 5: Commit des modifications..."
git commit -m "hotfix: correction erreur GPU Qt WebEngine + Qt par défaut

- Force le rendu logiciel pour Qt WebEngine si GPU non disponible
- Désactive Vulkan explicitement
- Support de cmake . à la racine avec détection auto Qt/GTK
- Qt6 est maintenant utilisé par défaut (au lieu de GTK)
- Améliore make help avec commandes Qt
- Ajoute nlohmann/ et ui-qt/build/ au .gitignore
- Fix: QRhiGles2 Failed to create context"
echo "✅ Commit créé"
echo ""

echo "✅ Étape 6: Vérification finale..."
echo ""
echo "=== Derniers commits ==="
git log --oneline -3
echo ""
echo "=== Branche actuelle ==="
git branch --show-current
echo ""
echo "=== État du dépôt ==="
git status
echo ""
echo "🎉 Toutes les modifications ont été sauvegardées avec succès !"

