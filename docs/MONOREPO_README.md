# 🏗️ WeedlyWeb - Architecture Monorepo

## 📋 Vue d'ensemble

WeedlyWeb est organisé en **monorepo** avec une architecture modulaire permettant de partager une bibliothèque C++ indépendante de la plateforme (`core`) entre plusieurs interfaces utilisateur.

## 📁 Structure du Projet

```
weedlyweb/
├── core/                    # 📚 Bibliothèque C++ indépendante de la plateforme
│   ├── include/             # Headers publics
│   │   ├── browser/        # Logique métier du navigateur
│   │   ├── database/       # Gestion de base de données
│   │   ├── http/           # Gestion HTTP
│   │   ├── storage/        # Gestion des fichiers
│   │   ├── memory/         # Gestion mémoire
│   │   ├── favorites/      # Logique des favorites
│   │   ├── tabs/           # Logique des tabs
│   │   └── utils/          # Utilitaires
│   ├── src/                # Implémentations
│   └── CMakeLists.txt      # Build core
│
├── ui-gtk/                  # 🖥️ Application GTK (Linux Desktop)
│   ├── include/gtk/        # Classes spécifiques GTK
│   ├── src/gtk/            # Implémentations GTK
│   ├── main.cpp            # Point d'entrée GTK
│   └── CMakeLists.txt      # Build GTK
│
├── ui-qt/                   # 🖥️ Application Qt (Multi-plateforme)
│   ├── include/qt/         # Classes spécifiques Qt
│   ├── src/qt/             # Implémentations Qt
│   ├── main.cpp            # Point d'entrée Qt
│   └── CMakeLists.txt      # Build Qt
│
├── ui-android/              # 📱 Application Android
│   ├── app/                 # Application Android native
│   ├── jni/                 # JNI wrapper pour core
│   ├── CMakeLists.txt       # Build Android
│   └── build.gradle         # Gradle build
│
├── tests/                   # 🧪 Tests unitaires
│   ├── core/                # Tests de la bibliothèque core
│   └── ui-gtk/              # Tests de l'UI GTK
│
├── docs/                    # 📖 Documentation
│   ├── core/                # Documentation core
│   ├── ui-gtk/              # Documentation GTK
│   ├── ui-qt/               # Documentation Qt
│   └── ui-android/          # Documentation Android
│
├── CMakeLists.txt           # CMake racine
├── Makefile                 # Makefile racine
└── README.md                # Ce fichier
```

## 🎯 Composants Core

La bibliothèque `core/` contient toute la logique métier indépendante de la plateforme :

- **`browser/`** : Moteur de navigation, gestion des tabs (logique)
- **`database/`** : Accès SQLite
- **`http/`** : Requêtes HTTP (cURL)
- **`storage/`** : Gestion fichiers/JSON
- **`memory/`** : Gestion mémoire
- **`favorites/`** : Logique des favorites
- **`tabs/`** : Gestion des tabs (logique)
- **`utils/`** : Utilitaires (CVE, parsing URL, etc.)

### Interfaces Abstraites

Le core définit des interfaces abstraites pour permettre l'implémentation multi-plateforme :

- `IWebView` : Interface pour une vue web
- `IBrowserWindow` : Interface pour une fenêtre de navigateur

## 🖥️ Interfaces Utilisateur

### ui-gtk/ (GTK+3 / WebKit2GTK)

Application desktop Linux utilisant GTK+3 et WebKit2GTK.

**Dépendances :**
- `core/` (bibliothèque)
- GTK+3
- WebKit2GTK 4.1

**Classes principales :**
- `GtkBrowserWindow` : Fenêtre principale
- `GtkWebView` : Wrapper WebKit2GTK
- `GtkTabBar` : Barre d'tabs
- `GtkFavoritesBar` : Barre de favorites

### ui-qt/ (Qt6 / Qt WebEngine)

Application desktop multi-plateforme utilisant Qt6.

**Dépendances :**
- `core/` (bibliothèque)
- Qt6 (Core, Gui, Widgets, WebEngine)

**Classes principales :**
- `QtBrowserWindow` : Fenêtre principale
- `QtWebView` : Wrapper Qt WebEngine
- `QtTabBar` : Barre d'tabs
- `QtFavoritesBar` : Barre de favorites

### ui-android/ (Android Native)

Application Android native avec wrapper JNI vers core.

**Dépendances :**
- `core/` (bibliothèque via JNI)
- Android NDK
- JNI

**Composants :**
- `AndroidBrowserActivity` : Activity principale
- `AndroidWebView` : WebView Android
- `JNIWrapper` : Interface JNI

## 🚀 Build

### Build complet

```bash
# Build core + ui-gtk
make build

# Build uniquement core
cd core && cmake -B build && cmake --build build

# Build uniquement ui-gtk
cd ui-gtk && cmake -B build && cmake --build build
```

### Options CMake

```bash
# Build avec Qt
cmake -B build -DBUILD_UI_QT=ON

# Build sans GTK
cmake -B build -DBUILD_UI_GTK=OFF
```

## 📦 Dépendances

### Core
- C++17
- nlohmann/json (header-only)
- cURL
- SQLite3
- **Aucune dépendance GUI**

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

## 🔄 Migration depuis l'architecture actuelle

Voir `MIGRATION_PLAN.md` pour le plan de migration détaillé.

## 📝 Notes

- Le core est **complètement indépendant** de toute bibliothèque GUI
- Les interfaces abstraites permettent l'implémentation multi-plateforme
- Chaque UI implémente les interfaces selon sa plateforme
- Les tests unitaires sont dans `tests/`

## 🎯 Avantages de cette architecture

1. **Réutilisabilité** : Le code core est partagé entre toutes les UIs
2. **Maintenabilité** : Séparation claire des responsabilités
3. **Testabilité** : Le core peut être testé indépendamment
4. **Extensibilité** : Facile d'ajouter de nouvelles UIs (Qt, Android, etc.)
5. **Performance** : Pas de duplication de code

