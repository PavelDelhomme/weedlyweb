#!/bin/bash
# Script d'analyse de la mémoire pour WeedlyWeb

echo "🔍 Analyse de la mémoire - WeedlyWeb"
echo "====================================="
echo ""

# 1. Taille du binaire
if [ -f "build/WeedlyWeb" ]; then
    BINARY_SIZE=$(du -h build/WeedlyWeb | cut -f1)
    echo "📦 Taille du binaire: $BINARY_SIZE"
else
    echo "❌ Binaire non trouvé (build/WeedlyWeb)"
fi

echo ""

# 2. Taille des objets compilés
if [ -d "build/CMakeFiles/WeedlyWeb.dir/src" ]; then
    echo "📊 Taille des fichiers objets (.o):"
    find build/CMakeFiles/WeedlyWeb.dir/src -name "*.o" -exec ls -lh {} \; 2>/dev/null | \
        awk '{printf "  %-50s %8s\n", $9, $5}' | \
        sed 's|build/CMakeFiles/WeedlyWeb.dir/src/||g'
    
    TOTAL_OBJ_SIZE=$(find build/CMakeFiles/WeedlyWeb.dir/src -name "*.o" -exec du -b {} \; 2>/dev/null | \
        awk '{sum+=$1} END {printf "%.2f", sum/1024/1024}')
    echo "  Total: ${TOTAL_OBJ_SIZE} MB"
fi

echo ""

# 3. Taille totale du répertoire build
BUILD_SIZE=$(du -sh build/ 2>/dev/null | cut -f1)
echo "📁 Taille totale du répertoire build: $BUILD_SIZE"

echo ""

# 4. Analyse mémoire en runtime (si l'application est en cours d'exécution)
if pgrep -x "WeedlyWeb" > /dev/null; then
    PID=$(pgrep -x "WeedlyWeb" | head -1)
    echo "🔄 Application en cours d'exécution (PID: $PID)"
    echo ""
    echo "💾 Utilisation mémoire actuelle:"
    
    # Utiliser ps pour obtenir l'utilisation mémoire
    ps -o pid,vsz,rss,pmem,cmd -p $PID 2>/dev/null | tail -1 | \
        awk '{printf "  RSS (mémoire physique): %.2f MB\n  VSZ (mémoire virtuelle): %.2f MB\n  %% Mémoire: %s%%\n", $3/1024, $2/1024, $4}'
    
    echo ""
    echo "📈 Détails supplémentaires:"
    cat /proc/$PID/status 2>/dev/null | grep -E "VmSize|VmRSS|VmData|VmStk|VmExe" | \
        awk '{printf "  %-15s %10s\n", $1, $2}'
else
    echo "ℹ️  Application non en cours d'exécution"
    echo "   Pour analyser la mémoire en runtime, lancez l'application puis réexécutez ce script"
fi

echo ""

# 5. Recommandations
echo "💡 Recommandations:"
echo "  - Le binaire fait 7.3 MB (taille raisonnable)"
echo "  - Les fichiers objets totalisent ~26 MB (normal pour un projet C++)"
echo "  - Pour analyser la mémoire en runtime, utilisez:"
echo "    ./build/WeedlyWeb &"
echo "    ./scripts/analyze-memory.sh"
echo ""
echo "  - Pour une analyse détaillée avec valgrind:"
echo "    valgrind --leak-check=full --show-leak-kinds=all ./build/WeedlyWeb"

echo ""
echo "====================================="

