#!/bin/bash
# Script pour générer une icône PNG à partir du SVG

set -e

ICON_DIR="assets/icons"
SVG_FILE="$ICON_DIR/weedlyweb.svg"
PNG_FILE="$ICON_DIR/weedlyweb.png"

echo "🎨 Génération de l'icône PNG depuis le SVG..."

# Vérifier si Inkscape est disponible
if command -v inkscape >/dev/null 2>&1; then
    echo "✅ Utilisation de Inkscape"
    inkscape "$SVG_FILE" --export-type=png --export-filename="$PNG_FILE" --export-width=256 --export-height=256
    echo "✅ Icône PNG générée : $PNG_FILE"
elif command -v convert >/dev/null 2>&1; then
    echo "✅ Utilisation de ImageMagick"
    convert -background none -resize 256x256 "$SVG_FILE" "$PNG_FILE"
    echo "✅ Icône PNG générée : $PNG_FILE"
elif command -v rsvg-convert >/dev/null 2>&1; then
    echo "✅ Utilisation de rsvg-convert"
    rsvg-convert -w 256 -h 256 "$SVG_FILE" -o "$PNG_FILE"
    echo "✅ Icône PNG générée : $PNG_FILE"
else
    echo "⚠️  Aucun outil de conversion trouvé"
    echo "💡 Installez l'un de ces outils :"
    echo "   - Inkscape : sudo pacman -S inkscape"
    echo "   - ImageMagick : sudo pacman -S imagemagick"
    echo "   - librsvg : sudo pacman -S librsvg"
    echo ""
    echo "📝 L'icône SVG est disponible : $SVG_FILE"
    echo "💡 Vous pouvez l'ouvrir avec un éditeur d'images et l'exporter en PNG"
    exit 1
fi

# Générer aussi différentes tailles pour les applications
if [ -f "$PNG_FILE" ]; then
    echo "📐 Génération des différentes tailles..."
    
    # 16x16 (favicon)
    if command -v convert >/dev/null 2>&1; then
        convert "$PNG_FILE" -resize 16x16 "$ICON_DIR/weedlyweb-16.png"
        convert "$PNG_FILE" -resize 32x32 "$ICON_DIR/weedlyweb-32.png"
        convert "$PNG_FILE" -resize 48x48 "$ICON_DIR/weedlyweb-48.png"
        convert "$PNG_FILE" -resize 64x64 "$ICON_DIR/weedlyweb-64.png"
        convert "$PNG_FILE" -resize 128x128 "$ICON_DIR/weedlyweb-128.png"
        echo "✅ Toutes les tailles générées"
    fi
    
    echo ""
    echo "✅ Icônes générées avec succès !"
    echo "📁 Fichiers créés dans : $ICON_DIR"
fi

