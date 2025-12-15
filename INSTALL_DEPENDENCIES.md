# 📦 Installation des Dépendances pour WeedlyWeb

## Système : Manjaro Linux

### Installation de CMake (obligatoire)

CMake est nécessaire pour compiler le projet. Installez-le avec :

```bash
sudo pacman -S cmake
```

### Installation des autres dépendances

Le projet nécessite également :

```bash
# WebKit2GTK (moteur de rendu)
sudo pacman -S webkit2gtk

# GTK+3 (interface graphique)
sudo pacman -S gtk3

# SQLite3 (base de données)
sudo pacman -S sqlite

# cURL (requêtes HTTP)
sudo pacman -S curl

# Outils de développement
sudo pacman -S base-devel pkg-config
```

### Installation en une seule commande

```bash
sudo pacman -S cmake webkit2gtk gtk3 sqlite curl base-devel pkg-config
```

### Vérification de l'installation

Après l'installation, vérifiez que tout est en place :

```bash
# Vérifier CMake
cmake --version

# Vérifier les bibliothèques
pkg-config --exists webkit2gtk-4.1 && echo "WebKit2GTK: OK" || echo "WebKit2GTK: MANQUANT"
pkg-config --exists sqlite3 && echo "SQLite3: OK" || echo "SQLite3: MANQUANT"
pkg-config --exists gtk+-3.0 && echo "GTK+3: OK" || echo "GTK+3: MANQUANT"
```

### Compilation

Une fois toutes les dépendances installées :

```bash
make build
```

ou

```bash
make clean build
```

### Alternative : Utiliser le Makefile pour vérifier les dépendances

Le Makefile inclut une commande pour vérifier les dépendances :

```bash
make check-deps
```

