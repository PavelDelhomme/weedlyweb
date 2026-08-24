# 🚀 Guide de démarrage rapide

Ce guide vous permettra de démarrer WeedlyWeb en quelques minutes.

## 📋 Prérequis

- Linux (Arch/Manjaro, Ubuntu/Debian, Fedora)
- Git installé
- Accès sudo pour installer les dépendances

## ⚡ Installation rapide

### 1. Cloner le projet

```bash
git clone https://github.com/PavelDelhomme/weedlyweb.git
cd weedlyweb
```

### 2. Configuration complète

```bash
# Tout en une commande (installe les dépendances + compile)
make setup
```

**Ou étape par étape :**
```bash
# Installer les dépendances
make install-deps

# Compiler
make build
```

### 3. Lancer l'application

```bash
# Recommandé : lanceur multi-backend (Wayland / X11)
make run

# Ou binaire direct :
./build/WeedlyWeb

# Backend Qt6 (si compilé) :
make run-qt
```

La fenêtre s'ouvre **maximisée sur l'écran où se trouve la souris** (multi-moniteurs).

## 🎯 Commandes essentielles

| Commande | Description |
|----------|-------------|
| `make build` | Compiler GTK (+ Qt si disponible) |
| `make run` / `make run-gtk` | Compiler et lancer GTK |
| `make run-qt` | Lancer le backend Qt6 |
| `make dev` | Mode développement (recompilation automatique) |
| `make clean` | Nettoyer le répertoire de build |
| `make check-deps` | Vérifier les dépendances installées |
| `make stop` | Arrêter les processus WeedlyWeb |
| `make help` | Afficher toutes les commandes disponibles |

## ⌨️ Raccourcis utiles

| Raccourci | Action |
|-----------|--------|
| `Ctrl+T` | Nouvel onglet |
| `Ctrl+W` | Fermer l'onglet |
| `F11` | Plein écran sur le moniteur courant |
| `Ctrl+Shift+C` | Palette de commandes |

Liste complète : [KEYBOARD_SHORTCUTS.md](KEYBOARD_SHORTCUTS.md)

## 🐛 En cas de problème

### Erreur de dépendances manquantes

```bash
make check-deps
./scripts/install-deps.sh
```

### Erreur de compilation

```bash
make clean
make build
```

### L'application ne démarre pas

```bash
ls -lh build/WeedlyWeb
make run-debug
# ou
./scripts/run-weedlyweb.sh
```

### Fenêtre sur le mauvais écran

Placez la souris sur l'écran voulu avant `make run`. Voir [DISPLAY_AND_MONITORS.md](DISPLAY_AND_MONITORS.md).

## 📚 Documentation complète

- **[README.md](../README.md)** — Documentation principale
- **[COMPATIBILITY.md](COMPATIBILITY.md)** — Plateformes et backends
- **[INSTALL_DEPENDENCIES.md](INSTALL_DEPENDENCIES.md)** — Guide d'installation détaillé
- **[GUIDE_DEBUG.md](GUIDE_DEBUG.md)** — Guide de débogage

## ✅ Vérification finale

Une fois l'application lancée, vous devriez voir :

- ✅ Une fenêtre de navigateur maximisée sur votre écran
- ✅ Barre d'onglets, barre d'adresse, barre de favoris
- ✅ Boutons mode sombre (◐) et mode lecture (≡) dans la barre d'URL
- ✅ Page d'accueil (DuckDuckGo par défaut)

## 🎉 C'est prêt !

1. Taper une URL dans la barre d'adresse
2. Clic droit sur un onglet → favoris, épingler, renommer…
3. `Ctrl+T` pour un nouvel onglet
4. `F11` pour le plein écran sur un seul moniteur

---

**Besoin d'aide ?** Consultez le [README](../README.md) ou ouvrez une [issue GitHub](https://github.com/PavelDelhomme/weedlyweb/issues).
