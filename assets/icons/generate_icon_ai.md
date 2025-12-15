# 🎨 Génération d'icône pour WeedlyWeb avec IA

## Instructions pour générer une icône avec Midjourney ou autres IA

### Prompt suggéré pour Midjourney/DALL-E/Stable Diffusion

```
A modern minimalist browser icon, web browser application icon, 
circular badge style, blue gradient background (#4A90E2 to #357ABD), 
globe/earth representation with network nodes connected, 
clean modern design, flat design style, 256x256 pixels, 
high contrast, professional software icon, 
white accents, web browser symbol, 
--style raw --ar 1:1
```

### Variantes du prompt

**Style minimaliste :**
```
Minimalist web browser icon, circular, blue gradient, 
simple globe with network connections, clean lines, 
modern flat design, software icon, 256x256
```

**Style moderne :**
```
Modern browser icon, circular badge, blue tones, 
earth globe with web network overlay, 
minimalist design, professional app icon, 
high quality, 256x256 pixels
```

**Style avec "W" (pour WeedlyWeb) :**
```
Letter W in a circular browser icon, blue gradient background, 
globe and network elements, modern minimalist design, 
web browser application icon, 256x256
```

### Services recommandés

1. **Midjourney** (via Discord)
   - Utilisez le prompt ci-dessus
   - Paramètres : `--ar 1:1 --style raw`

2. **DALL-E 3** (via ChatGPT Plus ou Bing)
   - Prompt détaillé fonctionne bien
   - Génère directement en 1024x1024

3. **Stable Diffusion** (local ou en ligne)
   - Utilisez le modèle SDXL
   - Paramètres : 256x256 ou 512x512

4. **Leonardo.ai** (gratuit avec limites)
   - Bon pour les icônes d'applications
   - Style "App Icon" disponible

### Après génération

1. Téléchargez l'image générée
2. Convertissez en SVG ou PNG 256x256
3. Placez dans `assets/icons/weedlyweb.png` ou `weedlyweb.svg`
4. Mettez à jour le Makefile/CMakeLists.txt pour inclure l'icône

### Outils de conversion

```bash
# Convertir PNG en SVG (si nécessaire)
convert weedlyweb.png -resize 256x256 weedlyweb.svg

# Ou utiliser Inkscape
inkscape weedlyweb.png --export-type=svg --export-filename=weedlyweb.svg
```

