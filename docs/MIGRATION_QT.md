# 🚀 Migration vers Qt6 - Guide

## ✅ État actuel

La migration vers Qt6 a été initiée. La structure `ui-qt/` a été créée avec :

- ✅ `CMakeLists.txt` pour Qt6
- ✅ `main.cpp` avec QApplication
- ✅ `BrowserWindow.h/cpp` (QMainWindow)
- ✅ `WebView.h/cpp` (QWebEngineView)
- ✅ `TabBar.h/cpp`
- ✅ `NavigationBar.h/cpp`
- ✅ `FavoritesBar.h/cpp`
- ✅ `CommandPalette.h/cpp`

## 📦 Installation de Qt6

### Sur Arch/Manjaro :
```bash
sudo pacman -S qt6-base qt6-webengine
```

### Sur Ubuntu/Debian :
```bash
sudo apt install qt6-base-dev qt6-webengine-dev
```

## 🔨 Compilation

### Option 1 : Build Qt (recommandé)
```bash
make build-qt
make run-qt
```

### Option 2 : Build manuel
```bash
cd ui-qt
mkdir -p build
cd build
cmake ..
make
./WeedlyWebQt
```

## 🎯 Avantages de Qt

1. **Qt WebEngine** est plus stable que WebKit2GTK
2. **Meilleur rendu** des pages web modernes
3. **Multi-plateforme** (Linux, Windows, macOS)
4. **Meilleure documentation** et communauté active

## 📝 Prochaines étapes

1. Tester la compilation Qt
2. Adapter les managers (FavoritesManager, TabsManager, etc.) pour Qt
3. Implémenter les fonctionnalités manquantes
4. Migrer les données (favoris, configuration)

