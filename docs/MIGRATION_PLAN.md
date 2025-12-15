# 📋 Plan de Migration vers Monorepo

## 🎯 Objectif

Transformer le projet actuel en monorepo avec :
- `core/` : Bibliothèque C++ indépendante
- `ui-gtk/` : Application GTK (actuelle)
- `ui-qt/` : Application Qt (futur)
- `ui-android/` : Application Android (futur)

## 📝 Étapes de Migration

### Étape 1 : Créer la structure de base

```bash
# Créer les dossiers
mkdir -p core/{include,src}/{browser,database,http,storage,memory,favorites,tabs,utils}
mkdir -p ui-gtk/{include,src}/gtk
mkdir -p ui-qt/{include,src}/qt
mkdir -p ui-android/{app,jni}
```

### Étape 2 : Identifier les composants Core

#### ✅ Composants à déplacer dans `core/` :

1. **`core/database/Database`** (déjà indépendant)
   - `include/database/Database.h` → `core/include/database/Database.h`
   - `src/database/Database.cpp` → `core/src/database/Database.cpp`

2. **`core/http/HttpManager`**
   - `include/HTTPManager.h` → `core/include/http/HttpManager.h`
   - `src/HTTPManager.cpp` → `core/src/http/HttpManager.cpp`
   - Retirer les dépendances GTK si présentes

3. **`core/storage/FileManager`**
   - `include/FileManager.h` → `core/include/storage/FileManager.h`
   - `src/FileManager.cpp` → `core/src/storage/FileManager.cpp`

4. **`core/memory/MemoryManager`**
   - `include/MemoryManager.h` → `core/include/memory/MemoryManager.h`
   - `src/MemoryManager.cpp` → `core/src/memory/MemoryManager.cpp`
   - Adapter pour être indépendant de WebKit

5. **`core/favorites/FavoritesManager`**
   - `include/FavoritesManager.h` → `core/include/favorites/FavoritesManager.h`
   - `src/FavoritesManager.cpp` → `core/src/favorites/FavoritesManager.cpp`
   - Retirer la partie UI GTK

6. **`core/tabs/TabManager`**
   - `include/TabsManager.h` → `core/include/tabs/TabManager.h`
   - `src/TabsManager.cpp` → `core/src/tabs/TabManager.cpp`
   - Retirer la partie UI GTK

7. **`core/utils/CVEAnalyzer`**
   - `include/utils/CVEAnalyzer.h` → `core/include/utils/CVEAnalyzer.h`
   - `src/utils/CVEAnalyzer.cpp` → `core/src/utils/CVEAnalyzer.cpp`

8. **`core/browser/BrowserEngine`** (nouveau)
   - Créer une classe qui orchestre les composants core
   - Logique métier sans UI

### Étape 3 : Créer les interfaces abstraites

Créer `core/include/browser/IWebView.h` et `core/include/browser/IBrowserWindow.h` pour permettre l'implémentation multi-plateforme.

### Étape 4 : Adapter ui-gtk/

1. Créer `ui-gtk/include/gtk/GtkBrowserWindow.h` (remplace `Browser.h`)
2. Créer `ui-gtk/include/gtk/GtkWebView.h` (remplace `RenderingEngine.h`)
3. Adapter pour utiliser `core/` via les interfaces abstraites

### Étape 5 : CMakeLists.txt racine

Créer un CMakeLists.txt racine qui gère les sous-projets.

## 🔧 Commandes de Migration

```bash
# 1. Créer la structure
./scripts/create_monorepo_structure.sh

# 2. Extraire les composants core
./scripts/extract_core_components.sh

# 3. Adapter ui-gtk
./scripts/adapt_ui_gtk.sh

# 4. Compiler et tester
make build
```

## ⚠️ Points d'attention

1. **Dépendances circulaires** : Éviter les dépendances entre core et ui-*
2. **Interfaces abstraites** : Créer des interfaces pures pour la réutilisation
3. **Tests** : Créer des tests unitaires pour core
4. **Documentation** : Documenter les interfaces core

