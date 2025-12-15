# 🌐 WeedlyWeb

**Un navigateur web simple et léger écrit en C++ avec GTK3 et WebKit2GTK**

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![C++](https://img.shields.io/badge/C++-17-blue.svg)](https://isocpp.org/)
[![GTK](https://img.shields.io/badge/GTK-3.0-green.svg)](https://www.gtk.org/)
[![WebKit](https://img.shields.io/badge/WebKit-2GTK%204.1-orange.svg)](https://webkitgtk.org/)

## 📋 Table des matières

- [Fonctionnalités](#-fonctionnalités)
- [Prérequis](#-prérequis)
- [Installation](#-installation)
- [Compilation](#-compilation)
- [Utilisation](#-utilisation)
- [Structure du projet](#-structure-du-projet)
- [Documentation](#-documentation)
- [Développement](#-développement)
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

## 📦 Prérequis

### Système d'exploitation

- **Linux** (testé sur Manjaro/Arch Linux, Ubuntu/Debian, Fedora)
- **GTK3** et **WebKit2GTK 4.1** doivent être installés

### Dépendances requises

- `cmake` (>= 3.16)
- `make` ou `ninja`
- `g++` (support C++17)
- `pkg-config`
- `webkit2gtk` (>= 4.1)
- `gtk3`
- `sqlite3`
- `curl`
- `gdk-pixbuf2`

## 🚀 Installation

### Installation automatique (Recommandé)

Un script d'installation automatique est disponible :

```bash
# Cloner le projet
git clone https://github.com/PavelDelhomme/weedlyweb.git
cd weedlyweb

# Installer les dépendances
./scripts/install-deps.sh
```

### Installation manuelle

#### Sur Arch Linux / Manjaro

```bash
sudo pacman -S cmake webkit2gtk gtk3 sqlite curl base-devel pkg-config gdk-pixbuf2
```

#### Sur Ubuntu / Debian

```bash
sudo apt-get update
sudo apt-get install cmake libwebkit2gtk-4.1-dev libgtk-3-dev libsqlite3-dev libcurl4-openssl-dev build-essential pkg-config libgdk-pixbuf2.0-dev
```

#### Sur Fedora

```bash
sudo dnf install cmake webkit2gtk-devel gtk3-devel sqlite-devel libcurl-devel gcc-c++ pkg-config gdk-pixbuf2-devel
```

### Vérification des dépendances

```bash
make check-deps
```

## 🔨 Compilation

### Compilation standard

```bash
# Compiler le projet
make build

# Ou simplement
make
```

### Compilation et lancement

```bash
# Compiler et lancer l'application
make run
```

### Mode développement (recompilation automatique)

```bash
# Surveille les fichiers et recompile automatiquement
make dev
```

### Autres commandes utiles

```bash
make help          # Afficher toutes les commandes disponibles
make clean          # Nettoyer le répertoire de build
make run-debug      # Lancer en mode debug
make debug          # Lancer avec GDB
make install        # Installer l'application
```

## 💻 Utilisation

### Lancement de l'application

```bash
# Après compilation
./build/WeedlyWeb

# Ou via make
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

### Fonctionnalités principales

1. **Navigation** : Tapez une URL dans la barre d'adresse et appuyez sur Entrée
2. **Favoris** : Cliquez sur l'étoile (☆) pour ajouter la page actuelle aux favoris
3. **Onglets** : Cliquez sur le bouton "+" pour créer un nouvel onglet
4. **Menu** : Cliquez sur l'icône hamburger (☰) pour accéder aux options

## 📁 Structure du projet

```
weedlyweb/
├── assets/              # Ressources (icônes, styles, données)
│   ├── datas/          # Fichiers JSON (favoris, config)
│   ├── icons/          # Icônes de l'application
│   ├── settings/       # Pages de paramètres
│   └── styles/         # Fichiers CSS
├── build/              # Répertoire de compilation (généré)
├── docs/               # Documentation
├── include/            # Fichiers d'en-tête
│   ├── browser/       # Classes principales du navigateur
│   ├── managers/      # Gestionnaires (favoris, onglets, etc.)
│   ├── rendering/     # Moteur de rendu WebKit
│   ├── utils/         # Utilitaires
│   └── ...
├── scripts/            # Scripts d'aide
├── src/                # Code source
│   ├── browser/       # Implémentation du navigateur
│   ├── managers/      # Implémentations des gestionnaires
│   ├── rendering/     # Implémentation du moteur de rendu
│   └── ...
├── CMakeLists.txt      # Configuration CMake
├── Makefile           # Makefile principal
└── README.md          # Ce fichier
```

## 📚 Documentation

Toute la documentation est disponible dans le dossier `docs/` :

- **[INSTALL_DEPENDENCIES.md](docs/INSTALL_DEPENDENCIES.md)** - Guide d'installation des dépendances
- **[GUIDE_DEBUG.md](docs/GUIDE_DEBUG.md)** - Guide de débogage
- **[ARCHITECTURE_MONOREPO.md](docs/ARCHITECTURE_MONOREPO.md)** - Architecture du projet
- **[STATUS.md](docs/STATUS.md)** - État actuel du projet

## 🛠️ Développement

### Configuration de l'environnement de développement

```bash
# Installer les outils de développement
sudo pacman -S gdb valgrind strace inotify-tools  # Arch/Manjaro
# ou
sudo apt-get install gdb valgrind strace inotify-tools  # Ubuntu/Debian
```

### Mode développement

Le mode développement surveille automatiquement les modifications et recompile :

```bash
make dev
```

### Débogage

```bash
# Lancer avec GDB
make debug

# Lancer avec Valgrind (détection de fuites mémoire)
make valgrind
```

### Tests

```bash
# Vérifier les dépendances
make check-deps

# Compiler en mode debug
make build-debug
```

## 🤝 Contribution

Les contributions sont les bienvenues ! Voici comment contribuer :

1. **Fork** le projet
2. Créer une branche pour votre fonctionnalité (`git checkout -b feat/ma-fonctionnalite`)
3. **Commit** vos changements (`git commit -m 'Ajout d'une fonctionnalité'`)
4. **Push** vers la branche (`git push origin feat/ma-fonctionnalite`)
5. Ouvrir une **Pull Request**

### Standards de code

- Code en **anglais** (noms de variables, fonctions, commentaires)
- Suivre la structure existante du projet
- Ajouter des commentaires pour les parties complexes
- Tester avant de soumettre une PR

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

1. Vérifiez la [documentation](docs/)
2. Consultez les [issues existantes](https://github.com/PavelDelhomme/weedlyweb/issues)
3. Créez une nouvelle issue avec les détails du problème

---

**Fait avec ❤️ pour la communauté open source**

