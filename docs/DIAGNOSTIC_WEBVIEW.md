# 🔍 Diagnostic : Problème d'affichage WebView

## Situation actuelle
- **Framework UI** : GTK+3
- **Moteur de rendu** : WebKit2GTK 4.1
- **Problème** : La WebView est créée mais le contenu HTML/CSS/JavaScript ne s'affiche pas

## Causes possibles

1. **WebView sans contexte de rendu valide**
   - La WebView doit être "réalisée" (realized) avant de charger du contenu
   - Le contexte de rendu GTK doit être initialisé

2. **Taille invalide**
   - La WebView doit avoir une taille valide (> 0x0) avant le chargement
   - WebKit nécessite une taille valide pour initialiser le contexte de rendu

3. **Hiérarchie de widgets incorrecte**
   - La WebView doit être dans un container visible et expansible
   - Tous les parents doivent être visibles

4. **Timing de chargement**
   - L'URL ne doit pas être chargée avant que la WebView soit complètement initialisée

## Solution proposée

### Option 1 : Corriger GTK (recommandé en premier)
- S'assurer que la WebView est "réalisée" avant le chargement
- Attendre le signal "realize" avant de charger l'URL
- Forcer une taille minimale valide

### Option 2 : Passer à Qt
- Qt WebEngine est généralement plus stable que WebKit2GTK
- Meilleure compatibilité multi-plateforme
- Nécessite une refactorisation importante du code

