# 🏗️ Architecture Monorepo - WeedlyWeb

## 📋 Vue d'ensemble

Structure monorepo modulaire permettant de partager une bibliothèque C++ indépendante de la plateforme (`core`) entre plusieurs interfaces utilisateur.

```
weedlyweb/
├── core/                    # Bibliothèque C++ indépendante de la plateforme
│   ├── include/
│   │   ├── browser/        # Logique métier du navigateur
│   │   ├── database/        # Gestion de database
│   │   ├── http/           # Gestion HTTP
│   │   ├── storage/        # Gestion des fichiers
│   │   ├── memory/         # Gestion mémoire
│   │   ├── favorites/      # Logique des favorites
│   │   ├── tabs/           # Logique des tabs
│   │   └── utils/          # Utilitaires
│   ├── src/
│   │   └── (mêmes sous-dossiers)
│   └── CMakeLists.txt
│
├── ui-gtk/                  # Application GTK (actuelle)
│   ├── include/
│   │   └── gtk/            # Classes spécifiques GTK
│   ├── src/
│   │   └── gtk/            # Implémentations GTK
│   ├── main.cpp
│   └── CMakeLists.txt
│
├── ui-qt/                   # Application Qt (futur)
│   ├── include/
│   │   └── qt/             # Classes spécifiques Qt
│   ├── src/
│   │   └── qt/             # Implémentations Qt
│   ├── main.cpp
│   └── CMakeLists.txt
│
├── ui-android/              # Application Android (futur)
│   ├── app/                 # Application Android native
│   ├── jni/                 # JNI wrapper pour core
│   ├── CMakeLists.txt
│   └── build.gradle
│
├── CMakeLists.txt           # CMake racine
├── Makefile                 # Makefile racine
└── README.md
```

## 🎯 Composants Core (Indépendants de la plateforme)

### ✅ À extraire dans `core/`

1. **`core/browser/`**
   - `BrowserEngine` : Logique métier du navigateur (sans UI)
   - `TabManager` : Gestion des tabs (logique)
   - `NavigationHistory` : Historique de navigation

2. **`core/database/`**
   - `Database` : Gestion SQLite (déjà indépendant)

3. **`core/http/`**
   - `HttpManager` : Requêtes HTTP (cURL)
   - `CertificateValidator` : Validation SSL

4. **`core/storage/`**
   - `FileManager` : Gestion fichiers/JSON
   - `ConfigManager` : Configuration

5. **`core/memory/`**
   - `MemoryManager` : Gestion mémoire (logique)

6. **`core/favorites/`**
   - `FavoritesManager` : Logique des favorites (sans UI)

7. **`core/tabs/`**
   - `TabGroup` : Groupes d'tabs (logique)

8. **`core/utils/`**
   - `CVEAnalyzer` : Analyse CVE
   - `UrlParser` : Parsing d'URL
   - `StringUtils` : Utilitaires strings

## 🎨 Composants UI (Spécifiques à la plateforme)

### `ui-gtk/` (GTK+3 / WebKit2GTK)

- `GtkBrowserWindow` : Fenêtre principale GTK
- `GtkWebView` : Wrapper WebKit2GTK
- `GtkTabBar` : Barre d'tabs GTK
- `GtkFavoritesBar` : Barre de favorites GTK
- `GtkCommandPalette` : Palette de commandes GTK

### `ui-qt/` (Qt6 / Qt WebEngine)

- `QtBrowserWindow` : Fenêtre principale Qt
- `QtWebView` : Wrapper Qt WebEngine
- `QtTabBar` : Barre d'tabs Qt
- `QtFavoritesBar` : Barre de favorites Qt
- `QtCommandPalette` : Palette de commandes Qt

### `ui-android/` (Android Native / Qt Android)

- `AndroidBrowserActivity` : Activity principale
- `AndroidWebView` : WebView Android
- `JNIWrapper` : Interface JNI vers core
- `AndroidTabManager` : Gestion tabs Android

## 🔄 Interfaces Abstraites

Pour permettre la réutilisation du code core, créer des interfaces abstraites :

```cpp
// core/include/browser/IWebView.h
class IWebView {
public:
    virtual ~IWebView() = default;
    virtual void loadUrl(const std::string& url) = 0;
    virtual std::string getCurrentUrl() const = 0;
    virtual std::string getTitle() const = 0;
};

// core/include/browser/IBrowserWindow.h
class IBrowserWindow {
public:
    virtual ~IBrowserWindow() = default;
    virtual void show() = 0;
    virtual void setTitle(const std::string& title) = 0;
    virtual IWebView* getWebView() = 0;
};
```

## 📦 Dépendances

### Core
- C++17
- nlohmann/json
- cURL
- SQLite3
- (Aucune dépendance GUI)

### ui-gtk
- Core (bibliothèque)
- GTK+3
- WebKit2GTK 4.1

### ui-qt
- Core (bibliothèque)
- Qt6 (Core, Gui, Widgets, WebEngine)

### ui-android
- Core (bibliothèque)
- Android NDK
- JNI
- (Optionnel) Qt Android

## 🚀 Plan de Migration

### Phase 1 : Préparation
1. Créer la structure de dossiers
2. Créer les CMakeLists.txt de base
3. Identifier et extraire les composants core

### Phase 2 : Extraction Core
1. Déplacer les composants indépendants
2. Créer les interfaces abstraites
3. Adapter les dépendances

### Phase 3 : Refactoring UI-GTK
1. Adapter `ui-gtk/` pour utiliser `core/`
2. Créer les wrappers GTK
3. Tester la compilation

### Phase 4 : UI-Qt (futur)
1. Créer `ui-qt/` avec Qt6
2. Implémenter les interfaces abstraites
3. Adapter le code core

### Phase 5 : UI-Android (futur)
1. Créer `ui-android/` avec JNI
2. Wrapper Android pour core
3. Interface native Android

