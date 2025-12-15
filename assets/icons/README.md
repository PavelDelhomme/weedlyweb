# 🎨 Icônes WeedlyWeb

## Icône actuelle

L'icône actuelle (`weedlyweb.svg` et `weedlyweb.png`) représente :
- Un globe terrestre avec des lignes de latitude/longitude
- Des nœuds réseau connectés (représentant le web)
- Un design moderne et minimaliste
- Des couleurs bleues (#4A90E2 à #357ABD)

## Générer une meilleure icône avec IA

Consultez `generate_icon_ai.md` pour des instructions détaillées sur la génération d'une icône avec Midjourney, DALL-E, ou d'autres services d'IA.

### Prompt recommandé pour Midjourney

```
A modern minimalist browser icon, web browser application icon, 
circular badge style, blue gradient background (#4A90E2 to #357ABD), 
globe/earth representation with network nodes connected, 
clean modern design, flat design style, 256x256 pixels, 
high contrast, professional software icon, 
white accents, web browser symbol, 
--style raw --ar 1:1
```

## Utilisation

L'icône est automatiquement chargée par l'application au démarrage. Elle apparaît :
- Dans la barre des tâches
- Dans la barre de titre de la fenêtre
- Dans le gestionnaire de fenêtres

## Génération des différentes tailles

Utilisez le script `scripts/generate_icon.sh` pour générer toutes les tailles nécessaires :

```bash
./scripts/generate_icon.sh
```

Cela génère :
- `weedlyweb-16.png` (favicon)
- `weedlyweb-32.png`
- `weedlyweb-48.png`
- `weedlyweb-64.png`
- `weedlyweb-128.png`
- `weedlyweb.png` (256x256)

