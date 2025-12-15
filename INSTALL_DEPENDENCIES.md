# 📦 Installation des Dépendances pour WeedlyWeb

## Système : Manjaro Linux (Arch Linux)

### Installation automatique (recommandé)

Un script d'installation automatique est disponible :

```bash
./install-deps.sh
```

Ce script vous guidera à travers l'installation et vous proposera d'installer les outils de débogage.

### Installation complète en une seule commande

Pour installer toutes les dépendances nécessaires (compilation + débogage) :

```bash
sudo pacman -S cmake webkit2gtk gtk3 sqlite curl base-devel pkg-config gdb valgrind strace
```

### Dépendances par catégorie

#### 🔧 Outils de compilation (obligatoires)

```bash
sudo pacman -S cmake base-devel pkg-config
```

#### 📚 Bibliothèques principales (obligatoires)

```bash
# WebKit2GTK 4.1 (moteur de rendu web)
sudo pacman -S webkit2gtk

# GTK+3 (interface graphique)
sudo pacman -S gtk3

# SQLite3 (base de données)
sudo pacman -S sqlite

# cURL (requêtes HTTP)
sudo pacman -S curl
```

#### 🐞 Outils de débogage (recommandés)

```bash
# GDB (débogueur)
sudo pacman -S gdb

# Valgrind (détection de fuites mémoire)
sudo pacman -S valgrind

# strace (traçage des appels système)
sudo pacman -S strace
```

### Installation minimale (sans outils de débogage)

Si vous ne voulez que compiler et lancer l'application :

```bash
sudo pacman -S cmake webkit2gtk gtk3 sqlite curl base-devel pkg-config
```

### Vérification de l'installation

Après l'installation, vérifiez que tout est en place :

```bash
# Vérifier CMake
cmake --version

# Vérifier les bibliothèques principales
pkg-config --exists webkit2gtk-4.1 && echo "✅ WebKit2GTK: OK" || echo "❌ WebKit2GTK: MANQUANT"
pkg-config --exists sqlite3 && echo "✅ SQLite3: OK" || echo "❌ SQLite3: MANQUANT"
pkg-config --exists gtk+-3.0 && echo "✅ GTK+3: OK" || echo "❌ GTK+3: MANQUANT"

# Vérifier les outils de débogage
command -v gdb >/dev/null 2>&1 && echo "✅ GDB: OK" || echo "⚠️  GDB: Non installé (optionnel)"
command -v valgrind >/dev/null 2>&1 && echo "✅ Valgrind: OK" || echo "⚠️  Valgrind: Non installé (optionnel)"
command -v strace >/dev/null 2>&1 && echo "✅ strace: OK" || echo "⚠️  strace: Non installé (optionnel)"
```

### Utilisation du Makefile pour vérifier les dépendances

Le Makefile inclut une commande pour vérifier automatiquement les dépendances :

```bash
make check-deps
```

Cette commande vérifie :
- ✅ CMake
- ✅ WebKit2GTK 4.1
- ✅ GTK+3
- ✅ cURL

### Compilation

Une fois toutes les dépendances installées :

```bash
# Compilation normale
make build

# Ou nettoyage + compilation
make clean build

# Compilation en mode debug (avec symboles)
make build-debug
```

### Lancement de l'application

```bash
# Lancement normal
make run

# Lancement en arrière-plan
make run-bg

# Lancement en mode debug
make run-debug
```

### Débogage

Si vous avez installé les outils de débogage :

```bash
# Déboguer avec GDB
make debug

# Détecter les fuites mémoire avec Valgrind
make valgrind
```

### Notes importantes

- **WebKit2GTK 4.1** : Assurez-vous d'avoir la version 4.1, pas une version antérieure
- **GTK+3** : Fonctionne sur tous les environnements de bureau (KDE, XFCE, GNOME, etc.)
- **GDB** : Optionnel mais recommandé pour déboguer les crashes
- **Valgrind** : Optionnel mais utile pour détecter les fuites mémoire
- **strace** : Optionnel, utile pour diagnostiquer les problèmes système

### Dépannage

Si vous rencontrez des erreurs de compilation :

1. Vérifiez que toutes les dépendances sont installées : `make check-deps`
2. Vérifiez la version de WebKit2GTK : `pkg-config --modversion webkit2gtk-4.1`
3. Vérifiez que les bibliothèques sont bien liées : `ldd build/WeedlyWeb | grep webkit`

