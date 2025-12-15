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
# Compiler et lancer en une commande
make run

# Ou si déjà compilé:
./build/WeedlyWeb
```

## 🎯 Commandes essentielles

| Commande | Description |
|----------|-------------|
| `make build` | Compiler le projet |
| `make run` | Compiler et lancer l'application |
| `make dev` | Mode développement (recompilation automatique) |
| `make clean` | Nettoyer le répertoire de build |
| `make check-deps` | Vérifier les dépendances installées |
| `make help` | Afficher toutes les commandes disponibles |

## 🐛 En cas de problème

### Erreur de dépendances manquantes

```bash
# Vérifier les dépendances
make check-deps

# Réinstaller les dépendances
./scripts/install-deps.sh
```

### Erreur de compilation

```bash
# Nettoyer et recompiler
make clean
make build
```

### L'application ne démarre pas

```bash
# Vérifier que l'exécutable existe
ls -lh build/WeedlyWeb

# Lancer avec debug
make run-debug
```

## 📚 Documentation complète

Pour plus d'informations, consultez :

- **[README.md](../README.md)** - Documentation principale
- **[INSTALL_DEPENDENCIES.md](INSTALL_DEPENDENCIES.md)** - Guide d'installation détaillé
- **[GUIDE_DEBUG.md](GUIDE_DEBUG.md)** - Guide de débogage

## ✅ Vérification finale

Une fois l'application lancée, vous devriez voir :

- ✅ Une fenêtre de navigateur
- ✅ Une barre d'adresse en haut
- ✅ Un bouton pour ajouter des onglets
- ✅ La page d'accueil (DuckDuckGo par défaut)

## 🎉 C'est prêt !

Vous pouvez maintenant utiliser WeedlyWeb. Essayez de :

1. Taper une URL dans la barre d'adresse
2. Cliquer sur l'étoile (☆) pour ajouter aux favoris
3. Utiliser CTRL+T pour ouvrir un nouvel onglet
4. Explorer les autres fonctionnalités !

---

**Besoin d'aide ?** Consultez la [documentation complète](../README.md) ou créez une [issue sur GitHub](https://github.com/PavelDelhomme/weedlyweb/issues).

