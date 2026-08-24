# 🌐 WeedlyWeb

**Un navigateur web simple et léger écrit en C++ avec GTK3 et WebKit2GTK**

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![C++](https://img.shields.io/badge/C++-17-blue.svg)](https://isocpp.org/)
[![GTK](https://img.shields.io/badge/GTK-3.0-green.svg)](https://www.gtk.org/)
[![WebKit](https://img.shields.io/badge/WebKit-2GTK%204.1-orange.svg)](https://webkitgtk.org/)

## 📋 Table des matières

- [Fonctionnalités](#-fonctionnalités)
- [Démarrage rapide](#-démarrage-rapide)
- [Installation](#-installation)
- [Compilation](#-compilation)
- [Utilisation](#-utilisation)
- [Développement](#-développement)
- [Architecture](#-architecture)
- [Structure du code](#-structure-du-code)
- [Standards de code](#-standards-de-code)
- [Workflow de développement](#-workflow-de-développement)
- [Documentation](#-documentation)
- [Contribution](#-contribution)

## ✨ Fonctionnalités

- 🌍 **Navigation web** avec WebKit2GTK 4.1 (GTK) ou Qt WebEngine (Qt6)
- 📑 **Onglets** avec groupes, menu contextuel (renommer, dupliquer, épingler, favoris)
- ⭐ **Favoris** avec dossiers/sous-dossiers et barre dédiée
- 🌙 **Mode sombre** des pages (type Dark Reader) et **mode lecture**
- 🖥️ **Multi-écrans** : lancement et plein écran (F11) sur le moniteur actif
- 🔍 **Barre d'URL** avec autocomplétion (favoris, historique, onglets)
- ⌨️ **Raccourcis globaux** (Ctrl+T, Ctrl+W, F11…) même dans la page web
- 🎨 **Interface** sombre, minimaliste
- 🔒 **Intercepteur de requêtes** et cookies persistants
- 📊 **SQLite** + JSON (favoris, historique, session)
- 🎯 **Palette de commandes** (Ctrl+Shift+C)

## 🚀 Démarrage rapide

> 💡 **Nouveau développeur ?** Consultez le [Guide de démarrage rapide](docs/QUICKSTART.md) pour une installation en 3 étapes !

```bash
# 1. Cloner le projet
git clone https://github.com/PavelDelhomme/weedlyweb.git
cd weedlyweb

# 2. Configuration complète (installe les dépendances + compile)
make setup

# 3. Lancer l'application
make run
```

**Ou étape par étape :**
```bash
make install-deps  # Installe les dépendances
make build         # Compile le projet
make run           # Lance l'application
```

## 📦 Installation

### Installation automatique (Recommandé)

Tout peut être fait via le Makefile :

```bash
# Cloner le projet
git clone https://github.com/PavelDelhomme/weedlyweb.git
cd weedlyweb

# Configuration complète (installe les dépendances + compile)
make setup
```

**Ou séparément :**
```bash
make install-deps  # Installe les dépendances (détection automatique de la distribution)
make build         # Compile le projet
```

### Installation manuelle

Voir [docs/INSTALL_DEPENDENCIES.md](docs/INSTALL_DEPENDENCIES.md) pour les instructions détaillées par distribution.

### Vérification des dépendances

```bash
make check-deps
```

## 🔨 Compilation

### Compilation standard

```bash
make build        # Compiler le projet
make run          # Compiler et lancer
```

### Mode développement

```bash
make dev          # Surveille les fichiers et recompile automatiquement
```

### Toutes les commandes disponibles

```bash
make help         # Aide Makefile
make build        # Compile GTK (+ Qt si installé)
make run          # Compile et lance GTK (multi-écran + lanceur Wayland/X11)
make run-gtk      # Idem GTK
make run-qt       # Backend Qt6
make run-debug    # Mode debug GTK
make clean        # Nettoie build/
make install      # Installation système
make install-deps # Dépendances (script)
make check-deps   # Vérifier deps
make dev          # Recompilation auto
make debug        # GDB
make valgrind     # Fuites mémoire
make stop         # Arrêter les processus WeedlyWeb
```

> 💡 **Tout peut être fait via le Makefile !** Utilisez `make help` pour voir toutes les commandes disponibles.

## 🖥️ Compatibilité affichage (X11 / Wayland / DE)

WeedlyWeb vise **Linux desktop** : GNOME, KDE Plasma, XFCE/Xubuntu, LXQt/Lubuntu, MATE, Cinnamon, i3, sway, dwm, etc.

Architecture multi-UI :

| Backend | Chemin | Commande |
|---------|--------|----------|
| GTK (défaut) | `src/ui/gtk` | `make run` / `make run-gtk` |
| Qt6 | `src/ui/qt` | `make run-qt` |
| Core partagé | `src/core` | — |
| Android / Cocoa / Win32 | stubs | futurs |

`make run` utilise `scripts/run-weedlyweb.sh` (backends GDK Wayland/X11).

Détails : [docs/COMPATIBILITY.md](docs/COMPATIBILITY.md) · [src/ui/README.md](src/ui/README.md).

> Windows / macOS natifs : stubs dans `src/ui/win32` et `src/ui/cocoa`. WSL2 / Homebrew en attendant.

## 💻 Utilisation

### Lancement

```bash
make run
# ou
./scripts/run-weedlyweb.sh
# Qt :
make run-qt
# binaire direct :
./build/WeedlyWeb
./build/WeedlyWebQt
# ou
make run
```

### Raccourcis clavier

Voir **[docs/KEYBOARD_SHORTCUTS.md](docs/KEYBOARD_SHORTCUTS.md)** pour la liste complète.

| Raccourci | Action |
|-----------|--------|
| `Ctrl+T` / `Ctrl++` | Nouvel onglet |
| `Ctrl+W` / `Ctrl+Shift+W` | Fermer l'onglet (dernier → quitte l'app) |
| `Ctrl+Shift+D` | Dupliquer l'onglet |
| `Ctrl+D` | Ajouter aux favoris |
| `Ctrl+L` | Focus barre d'URL |
| `Ctrl+R` / `F5` | Actualiser |
| `Ctrl+Shift+C` | Palette de commandes |
| `F11` | Plein écran sur le moniteur courant |

**Clic droit sur un onglet** : fermer, dupliquer, renommer, épingler, favoris, déplacer vers un groupe.

### Affichage multi-écrans

Au lancement, la fenêtre s'ouvre **maximisée sur l'écran où se trouve la souris**.  
**F11** = plein écran sur **cet** écran uniquement (pas le bureau virtuel 3×).

Détails : [docs/DISPLAY_AND_MONITORS.md](docs/DISPLAY_AND_MONITORS.md)

## 🛠️ Développement

### Configuration de l'environnement

```bash
# Installer les outils de développement
./scripts/install-deps.sh  # Inclut les outils de debug si demandé

# Ou manuellement :
# Arch/Manjaro:
sudo pacman -S gdb valgrind strace inotify-tools

# Ubuntu/Debian:
sudo apt-get install gdb valgrind strace inotify-tools
```

### Mode développement

Le mode développement surveille automatiquement les modifications et recompile :

```bash
make dev
```

### Débogage

```bash
make debug        # Lancer avec GDB (interactif)
make debug-auto   # Lancer avec GDB (automatique)
make valgrind     # Détection de fuites mémoire
```

Voir [docs/GUIDE_DEBUG.md](docs/GUIDE_DEBUG.md) pour plus de détails.

### Tests

```bash
make check-deps   # Vérifier les dépendances
make build-debug  # Compiler en mode debug
```

## 🏗️ Architecture

```
weedlyweb/
├── src/
│   ├── core/              # Logique partagée (DB, HTTP, favoris JSON, onglets…)
│   └── ui/
│       ├── gtk/           # Backend principal → WeedlyWeb
│       ├── qt/            # Backend Qt6 → WeedlyWebQt
│       └── {android,cocoa,win32}/  # Stubs futurs
├── include/               # En-têtes (miroir + core/)
├── assets/                # CSS, config, favoris, icônes, .desktop
├── scripts/               # install-deps, run-weedlyweb.sh, weedlyweb
└── docs/                  # Documentation
```

CMake : `-DBUILD_UI_GTK=ON -DBUILD_UI_QT=ON` pour compiler les deux backends.

### Composants principaux

- **`Browser`** (GTK) : fenêtre, onglets, barres, session, raccourcis
- **`WeedlyWebCore`** : `FileManager`, `Database`, `TabsManager`, `FavoritesJson`, `PageEnhancements`
- **`RenderingEngine`** : WebKit2GTK
- **`FavoritesManager`** : UI favoris + dossiers
- **`BrowserWindow`** (Qt) : variante Qt6

Voir [src/ui/README.md](src/ui/README.md) et [docs/ARCHITECTURE_MONOREPO.md](docs/ARCHITECTURE_MONOREPO.md).

## 📁 Structure du code (détail GTK)

```
src/ui/gtk/
├── browser/Browser.cpp    # Application principale
├── rendering/             # WebKit
├── managers/              # Favoris UI, mémoire…
└── utils/                 # Palette, intercepteur…
```

## 📝 Standards de code

### Conventions de nommage

- **Classes** : `PascalCase` (ex: `Browser`, `RenderingEngine`)
- **Fonctions** : `camelCase` (ex: `addNewTab`, `loadURL`)
- **Variables** : `camelCase` (ex: `webView`, `favoritesBar`)
- **Constantes** : `UPPER_SNAKE_CASE` (ex: `MAX_TABS`)
- **Fichiers** : Même nom que la classe principale

### Langage

- **Code** : Anglais (noms, commentaires)
- **Documentation** : Français (README, docs)
- **Messages utilisateur** : Peuvent être en français

### Style de code

- **Indentation** : 4 espaces
- **Longueur de ligne** : 100 caractères max
- **Commentaires** : Expliquer le "pourquoi", pas le "comment"
- **Headers** : Include guards (`#ifndef CLASS_H`)

### Exemple

```cpp
// include/browser/Browser.h
#ifndef BROWSER_H
#define BROWSER_H

class Browser {
public:
    void addNewTab(const std::string& url = "");
    void loadURL(const std::string& url);
    
private:
    std::unique_ptr<RenderingEngine> renderingEngine;
};

#endif
```

## 🔄 Workflow de développement

### 1. Créer une branche

```bash
git checkout dev
git pull origin dev
git checkout -b feat/ma-fonctionnalite
```

### 2. Développer

```bash
# Mode développement (recompilation automatique)
make dev

# Ou compilation manuelle
make build
make run
```

### 3. Tester

```bash
# Vérifier que ça compile
make build

# Tester l'application
make run

# Déboguer si nécessaire
make debug
```

### 4. Commit

```bash
git add .
git commit -m "feat: Description de la fonctionnalité"
```

### 5. Push et Pull Request

```bash
git push origin feat/ma-fonctionnalite
```

Puis créer une Pull Request sur GitHub de `feat/ma-fonctionnalite` vers `dev`.

### Format des commits

Utiliser le format [Conventional Commits](https://www.conventionalcommits.org/) :

- `feat:` Nouvelle fonctionnalité
- `fix:` Correction de bug
- `docs:` Documentation
- `refactor:` Refactorisation
- `test:` Tests
- `chore:` Maintenance

## 📚 Documentation

Toute la documentation est dans `docs/` :

- **[QUICKSTART.md](docs/QUICKSTART.md)** — Démarrage rapide
- **[COMPATIBILITY.md](docs/COMPATIBILITY.md)** — X11, Wayland, distros, backends
- **[DISPLAY_AND_MONITORS.md](docs/DISPLAY_AND_MONITORS.md)** — Multi-écrans et F11
- **[KEYBOARD_SHORTCUTS.md](docs/KEYBOARD_SHORTCUTS.md)** — Raccourcis clavier
- **[INSTALL_DEPENDENCIES.md](docs/INSTALL_DEPENDENCIES.md)** — Dépendances
- **[GUIDE_DEBUG.md](docs/GUIDE_DEBUG.md)** — Débogage
- **[STATUS.md](docs/STATUS.md)** — État du projet

## 🤝 Contribution

Les contributions sont les bienvenues ! Voici comment contribuer :

1. **Fork** le projet
2. Créer une branche (`git checkout -b feat/ma-fonctionnalite`)
3. **Développer** en suivant les standards de code
4. **Tester** votre code
5. **Commit** avec un message clair
6. **Push** vers votre fork
7. Ouvrir une **Pull Request** vers `dev`

### Checklist avant PR

- [ ] Code compile sans erreurs
- [ ] Code suit les conventions de nommage
- [ ] Commentaires ajoutés pour les parties complexes
- [ ] Testé manuellement
- [ ] Pas de fuites mémoire (vérifié avec Valgrind si possible)
- [ ] Message de commit suit le format Conventional Commits

## 📝 Licence

Ce projet est sous licence MIT. Voir le fichier `LICENSE` pour plus de détails.

## 👤 Auteur

**PavelDelhomme**

- GitHub: [@PavelDelhomme](https://github.com/PavelDelhomme)
- Projet: [WeedlyWeb](https://github.com/PavelDelhomme/weedlyweb)

## 🙏 Remerciements

- WebKit2GTK pour le moteur de rendu
- GTK pour l'interface graphique
- Tous les contributeurs et testeurs

## 📞 Support

Si vous rencontrez des problèmes :

1. Consultez la [documentation](docs/)
2. Vérifiez les [issues existantes](https://github.com/PavelDelhomme/weedlyweb/issues)
3. Créez une nouvelle issue avec les détails du problème

---

**Fait avec ❤️ pour la communauté open source**
