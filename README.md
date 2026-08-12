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

- 🌍 **Navigation web** avec WebKit2GTK 4.1
- 📑 **Gestion des onglets** avec groupes d'onglets
- ⭐ **Favoris** avec organisation par catégories
- 🔍 **Barre d'adresse** avec auto-complétion
- ⌨️ **Raccourcis clavier** (CTRL+T, CTRL+D, etc.)
- 🎨 **Interface moderne** et minimaliste
- 🔒 **Sécurité** avec intercepteur de requêtes
- 📊 **Base de données SQLite** pour l'historique et les favoris
- 🎯 **Palette de commandes** (CTRL+ALT+C)

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
make help         # Afficher toutes les commandes disponibles
make setup        # Configuration complète (deps + build)
make install-deps # Installe les dépendances
make build        # Compile le projet
make clean        # Nettoyer le répertoire de build
make run          # Compile et lance l'application
make run-debug    # Lancer en mode debug
make debug        # Lancer avec GDB
make dev          # Mode développement (recompilation automatique)
make install      # Installer l'application
make check-deps   # Vérifier les dépendances installées
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

| Raccourci | Action |
|-----------|--------|
| `CTRL + T` | Ouvrir un nouvel onglet |
| `CTRL + D` | Ajouter aux favoris |
| `CTRL + SHIFT + D` | Dupliquer l'onglet actuel |
| `CTRL + ALT + C` | Afficher la palette de commandes |
| `CTRL + W` | Fermer l'onglet actuel |
| `CTRL + R` | Actualiser la page |
| `CTRL + H` | Aller à la page d'accueil |

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

Le projet est organisé en modules indépendants :

```
weedlyweb/
├── src/
│   ├── browser/         # Classe principale Browser
│   ├── rendering/        # Moteur de rendu WebKit (RenderingEngine)
│   ├── managers/         # Gestionnaires (Favorites, Tabs, HTTP, Memory, File)
│   ├── engine/           # Moteur de scripts (ScriptEngine)
│   ├── database/         # Gestion base de données SQLite
│   └── utils/           # Utilitaires (CommandPalette, RequestInterceptor, etc.)
├── include/              # Fichiers d'en-tête (même structure)
└── assets/               # Ressources (icônes, styles, données)
```

### Composants principaux

- **`Browser`** : Classe principale, orchestre tous les composants
- **`RenderingEngine`** : Gère WebKitWebView et le rendu des pages
- **`FavoritesManager`** : Gestion des favoris (ajout, suppression, organisation)
- **`TabsManager`** : Gestion des onglets et groupes d'onglets
- **`HTTPManager`** : Requêtes HTTP (cURL)
- **`MemoryManager`** : Gestion mémoire et nettoyage
- **`FileManager`** : Gestion fichiers et JSON
- **`Database`** : Accès SQLite3
- **`CommandPalette`** : Palette de commandes (CTRL+ALT+C)
- **`RequestInterceptor`** : Interception et filtrage des requêtes

## 📁 Structure du code

### Organisation des fichiers

- **Headers** : `include/<module>/<Class>.h`
- **Sources** : `src/<module>/<Class>.cpp`
- **Noms en anglais** : Tous les noms de classes, fonctions et variables sont en anglais

### Exemple de structure

```
include/
├── browser/
│   └── Browser.h
├── rendering/
│   └── RenderingEngine.h
└── managers/
    ├── FavoritesManager.h
    ├── TabsManager.h
    └── ...

src/
├── browser/
│   └── Browser.cpp
├── rendering/
│   └── RenderingEngine.cpp
└── managers/
    ├── FavoritesManager.cpp
    ├── TabsManager.cpp
    └── ...
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

- **[QUICKSTART.md](docs/QUICKSTART.md)** - Guide de démarrage rapide
- **[INSTALL_DEPENDENCIES.md](docs/INSTALL_DEPENDENCIES.md)** - Installation détaillée
- **[GUIDE_DEBUG.md](docs/GUIDE_DEBUG.md)** - Guide de débogage
- **[ARCHITECTURE_MONOREPO.md](docs/ARCHITECTURE_MONOREPO.md)** - Architecture du projet
- **[STATUS.md](docs/STATUS.md)** - État actuel du projet

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
