# 📊 État actuel : Framework UI

## Situation actuelle

**Framework utilisé** : **GTK+3** avec **WebKit2GTK 4.1**

Le projet démarre actuellement en **ui-gtk** (GTK+3), pas en ui-qt.

### Structure actuelle
```
src/main.cpp          → gtk_init() → GTK+3
src/browser/Browser.cpp → Utilise GTK widgets (GtkWindow, GtkBox, etc.)
src/rendering/RenderingEngine.cpp → Utilise WebKit2GTK
```

## Problème identifié

La WebView WebKit2GTK ne s'affiche pas correctement. Causes possibles :
1. La WebView doit être "réalisée" (realized) avant de charger du contenu
2. Le contexte de rendu GTK doit être initialisé
3. La WebView doit avoir une taille valide (> 0x0)

## Solutions appliquées

### Solution 1 : Vérification du signal "realize"
- Ajout d'une vérification `gtk_widget_get_realized()` avant de charger l'URL
- Connexion au signal "realize" pour charger l'URL une fois la WebView prête
- Taille minimale forcée (800x600) pour la WebView

### Solution 2 : Passage à Qt (si nécessaire)

Si le problème persiste avec GTK, passer à Qt serait une option. Qt WebEngine est généralement plus stable.

**Avantages de Qt :**
- Meilleure compatibilité multi-plateforme
- Qt WebEngine plus stable que WebKit2GTK
- Meilleure documentation et communauté

**Inconvénients :**
- Refactorisation importante du code
- Dépendances Qt à installer
- Changement de l'architecture actuelle

## Prochaines étapes

1. **Tester la solution actuelle** avec `make run-build`
2. Si ça ne fonctionne pas, **considérer le passage à Qt**
3. Créer une branche `feat/qt-migration` pour la migration

## Commandes utiles

```bash
# Tester la solution actuelle
make run-build

# Vérifier si Qt est disponible
pkg-config --exists Qt6Core Qt6Gui Qt6Widgets Qt6WebEngineWidgets

# Installer Qt6 (si nécessaire)
# Sur Arch/Manjaro:
sudo pacman -S qt6-base qt6-webengine

# Sur Ubuntu/Debian:
sudo apt install qt6-base-dev qt6-webengine-dev
```

