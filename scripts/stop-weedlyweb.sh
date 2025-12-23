#!/bin/bash
# Script pour arrêter tous les processus WeedlyWeb

echo "🛑 Arrêt de tous les processus WeedlyWeb..."
echo "=========================================="
echo ""

# Trouver tous les processus WeedlyWeb
PIDS=$(pgrep -f "WeedlyWeb" 2>/dev/null)

if [ -z "$PIDS" ]; then
    echo "✅ Aucun processus WeedlyWeb en cours d'exécution"
    exit 0
fi

echo "📋 Processus WeedlyWeb trouvés:"
ps -p $PIDS -o pid,cmd --no-headers 2>/dev/null | while read line; do
    echo "   $line"
done

echo ""
echo "🔄 Arrêt des processus..."

# Essayer d'abord un arrêt gracieux (SIGTERM)
for PID in $PIDS; do
    if kill -0 $PID 2>/dev/null; then
        echo "   Envoi de SIGTERM au processus $PID..."
        kill -TERM $PID 2>/dev/null
    fi
done

# Attendre un peu pour que les processus se terminent
sleep 2

# Vérifier s'il reste des processus
REMAINING=$(pgrep -f "WeedlyWeb" 2>/dev/null)

if [ -n "$REMAINING" ]; then
    echo ""
    echo "⚠️  Certains processus n'ont pas répondu à SIGTERM"
    echo "🔄 Envoi de SIGKILL (arrêt forcé)..."
    
    for PID in $REMAINING; do
        if kill -0 $PID 2>/dev/null; then
            echo "   Envoi de SIGKILL au processus $PID..."
            kill -KILL $PID 2>/dev/null
        fi
    done
    
    sleep 1
fi

# Vérification finale
FINAL_CHECK=$(pgrep -f "WeedlyWeb" 2>/dev/null)

if [ -z "$FINAL_CHECK" ]; then
    echo ""
    echo "✅ Tous les processus WeedlyWeb ont été arrêtés"
else
    echo ""
    echo "⚠️  Certains processus persistent:"
    ps -p $FINAL_CHECK -o pid,cmd --no-headers 2>/dev/null
    echo ""
    echo "💡 Essayez manuellement: kill -9 $FINAL_CHECK"
    exit 1
fi

echo ""
echo "=========================================="

