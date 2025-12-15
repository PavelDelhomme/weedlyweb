#!/bin/bash
# Script de débogage pour WeedlyWeb

echo "🔍 Débogage de WeedlyWeb..."
echo ""

# Vérifier les dépendances
echo "📦 Vérification des dépendances :"
pkg-config --modversion webkit2gtk-4.1 2>/dev/null && echo "✅ WebKit2GTK trouvé" || echo "❌ WebKit2GTK non trouvé"
pkg-config --modversion gtk+-3.0 2>/dev/null && echo "✅ GTK+3 trouvé" || echo "❌ GTK+3 non trouvé"
echo ""

# Vérifier les bibliothèques liées
echo "📚 Bibliothèques WebKit liées :"
ldd build/WeedlyWeb 2>/dev/null | grep webkit || echo "❌ Aucune bibliothèque WebKit trouvée"
echo ""

# Activer les messages de debug GTK
export G_MESSAGES_DEBUG=all
export WEBKIT_DISABLE_COMPOSITING_MODE=1

# Lancer avec strace pour voir les appels système (si disponible)
if command -v strace >/dev/null 2>&1; then
    echo "🔍 Lancement avec strace (dernières 20 lignes) :"
    timeout 3 strace -e trace=open,openat,mmap,munmap ./build/WeedlyWeb 2>&1 | tail -20
else
    echo "⚠️  strace non disponible, lancement normal avec messages de debug :"
    timeout 3 ./build/WeedlyWeb 2>&1 | head -30
fi

echo ""
echo "✅ Débogage terminé"

