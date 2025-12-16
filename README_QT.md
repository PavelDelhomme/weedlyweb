# 🚀 WeedlyWeb - Version Qt6

## Migration vers Qt6

Le projet a été migré vers **Qt6** avec **QWebEngineView** pour un meilleur rendu des pages web.

## Installation

### Dépendances Qt6

```bash
# Arch/Manjaro
sudo pacman -S qt6-base qt6-webengine

# Ubuntu/Debian
sudo apt install qt6-base-dev qt6-webengine-dev
```

## Compilation et exécution

### Méthode 1 : Makefile (recommandé)
```bash
make build-qt    # Compile avec Qt6
make run-qt      # Lance l'application Qt
```

### Méthode 2 : CMake direct
```bash
cd ui-qt
mkdir -p build
cd build
cmake ..
make
./WeedlyWebQt
```

## Structure

```
ui-qt/
├── CMakeLists.txt          # Configuration CMake pour Qt
├── include/qt/            # Headers Qt
│   ├── BrowserWindow.h
│   ├── WebView.h
│   ├── TabBar.h
│   ├── NavigationBar.h
│   ├── FavoritesBar.h
│   └── CommandPalette.h
└── src/qt/                # Sources Qt
    ├── main.cpp
    ├── BrowserWindow.cpp
    ├── WebView.cpp
    ├── TabBar.cpp
    ├── NavigationBar.cpp
    ├── FavoritesBar.cpp
    └── CommandPalette.cpp
```

## Avantages de Qt

- ✅ **Qt WebEngine** plus stable que WebKit2GTK
- ✅ **Meilleur rendu** des pages web modernes
- ✅ **Multi-plateforme** (Linux, Windows, macOS)
- ✅ **Meilleure documentation**

## Notes

- Les managers (FavoritesManager, TabsManager, etc.) sont partagés entre GTK et Qt
- La configuration est compatible entre les deux versions
- Les favoris sont partagés

