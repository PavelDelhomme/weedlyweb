#!/bin/bash
# Script pour lancer WeedlyWeb avec Valgrind pour détecter les fuites mémoire

echo "🔍 Lancement de WeedlyWeb avec Valgrind..."
echo "=========================================="
echo ""

# Vérifier que le binaire existe
if [ ! -f "build/WeedlyWeb" ]; then
    echo "❌ Erreur: Binaire non trouvé (build/WeedlyWeb)"
    echo "💡 Compilez d'abord avec: make"
    exit 1
fi

# Vérifier que valgrind est installé
if ! command -v valgrind &> /dev/null; then
    echo "❌ Erreur: Valgrind n'est pas installé"
    echo "💡 Installez-le avec: sudo pacman -S valgrind"
    exit 1
fi

# Créer le répertoire pour les logs si nécessaire
mkdir -p logs

# Créer le timestamp une seule fois
TIMESTAMP=$(date +%Y%m%d-%H%M%S)
LOG_FILE="logs/valgrind-${TIMESTAMP}.log"
OUTPUT_FILE="logs/valgrind-output-${TIMESTAMP}.txt"

# Options Valgrind recommandées pour GTK/WebKit
VALGRIND_OPTS="--leak-check=full"
VALGRIND_OPTS="$VALGRIND_OPTS --show-leak-kinds=all"
VALGRIND_OPTS="$VALGRIND_OPTS --track-origins=yes"
VALGRIND_OPTS="$VALGRIND_OPTS --track-fds=yes"
VALGRIND_OPTS="$VALGRIND_OPTS --verbose"
VALGRIND_OPTS="$VALGRIND_OPTS --log-file=$LOG_FILE"
VALGRIND_OPTS="$VALGRIND_OPTS --error-limit=no"
VALGRIND_OPTS="$VALGRIND_OPTS --num-callers=20"

# Ajouter les fichiers de suppression s'ils existent
if [ -f "/usr/share/gtk-3.0/valgrind.supp" ]; then
    VALGRIND_OPTS="$VALGRIND_OPTS --suppressions=/usr/share/gtk-3.0/valgrind.supp"
fi
if [ -f "/usr/share/glib-2.0/valgrind/glib.supp" ]; then
    VALGRIND_OPTS="$VALGRIND_OPTS --suppressions=/usr/share/glib-2.0/valgrind/glib.supp"
fi
if [ -f "$(dirname $0)/../valgrind.supp" ]; then
    VALGRIND_OPTS="$VALGRIND_OPTS --suppressions=$(dirname $0)/../valgrind.supp"
fi

# Note: Le syscall 317 est probablement memfd_create, utilisé par WebKit
# Valgrind peut afficher un warning mais cela n'empêche pas l'analyse
# On peut ignorer ce warning car il n'affecte pas la détection de fuites

# Variables d'environnement pour l'application
export WEBKIT_DISABLE_COMPOSITING_MODE=1
export LIBGL_ALWAYS_SOFTWARE=1

echo "📝 Fichier de log Valgrind: $LOG_FILE"
echo "📝 Fichier de sortie: $OUTPUT_FILE"
echo ""
echo "🚀 Démarrage de l'application avec Valgrind..."
echo "   (L'application va s'ouvrir dans une fenêtre)"
echo "   (Fermez l'application pour terminer l'analyse)"
echo ""
echo "ℹ️  Note: Un warning concernant le syscall 317 peut apparaître."
echo "   Ce n'est pas critique et n'affecte pas la détection de fuites mémoire."
echo "   C'est un syscall moderne (memfd_create) utilisé par WebKit."
echo ""
echo "💡 Pour voir la sortie en temps réel dans un autre terminal:"
echo "   tail -f $OUTPUT_FILE"
echo ""

# Lancer valgrind et afficher la sortie en temps réel
# Utiliser unbuffered si disponible pour voir la sortie immédiatement
if command -v stdbuf &> /dev/null; then
    valgrind $VALGRIND_OPTS ./build/WeedlyWeb 2>&1 | stdbuf -oL -eL tee "$OUTPUT_FILE"
else
    valgrind $VALGRIND_OPTS ./build/WeedlyWeb 2>&1 | tee "$OUTPUT_FILE"
fi

EXIT_CODE=${PIPESTATUS[0]}

echo ""
echo "=========================================="
if [ $EXIT_CODE -eq 0 ]; then
    echo "✅ Analyse Valgrind terminée"
else
    echo "⚠️  Application terminée avec le code: $EXIT_CODE"
fi
echo ""

# Afficher un résumé de l'analyse
if [ -f "$LOG_FILE" ]; then
    echo "📊 Résumé de l'analyse:"
    echo "   - Log Valgrind: $LOG_FILE"
    
    # Afficher un résumé des fuites détectées
    if grep -q "LEAK SUMMARY" "$LOG_FILE"; then
        echo ""
        echo "🔍 Résumé des fuites détectées:"
        grep -A 10 "LEAK SUMMARY" "$LOG_FILE" | head -15
    fi
    
    # Afficher le résumé des erreurs
    if grep -q "ERROR SUMMARY" "$LOG_FILE"; then
        echo ""
        echo "❌ Résumé des erreurs:"
        grep "ERROR SUMMARY" "$LOG_FILE"
    fi
    
    # Afficher les fuites définitives
    if grep -q "definitely lost" "$LOG_FILE"; then
        LOST=$(grep "definitely lost" "$LOG_FILE" | awk '{print $4}')
        if [ "$LOST" != "0" ] && [ "$LOST" != "0:" ]; then
            echo ""
            echo "⚠️  Fuites définitives détectées: $LOST"
        fi
    fi
fi

if [ -f "$OUTPUT_FILE" ]; then
    echo "   - Sortie complète: $OUTPUT_FILE"
fi

echo ""
echo "💡 Pour voir le rapport complet, consultez: $LOG_FILE"
