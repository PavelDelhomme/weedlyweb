# 📦 Installation des dépendances pour WeedlyWeb

## Systèmes supportés

- **Arch Linux / Manjaro**
- **Ubuntu / Debian**
- **Fedora**
- Autres distributions Linux avec GTK3 et WebKit2GTK

### Installation automatique (Recommandé)

Un script d'installation automatique est disponible :

```bash
./scripts/install-deps.sh
```

Ce script détecte automatiquement votre distribution (Arch/Manjaro, Ubuntu/Debian, Fedora) et vous guide à travers l'installation. Il propose également d'installer les outils de débogage et de surveillance.

### Installation complète en une commande

#### Arch Linux / Manjaro

```bash
sudo pacman -S cmake webkit2gtk gtk3 sqlite curl base-devel pkg-config gdk-pixbuf2 gdb valgrind strace inotify-tools
```

#### Ubuntu / Debian

```bash
sudo apt-get update
sudo apt-get install cmake libwebkit2gtk-4.1-dev libgtk-3-dev libsqlite3-dev libcurl4-openssl-dev build-essential pkg-config libgdk-pixbuf2.0-dev gdb valgrind strace inotify-tools
```

#### Fedora

```bash
sudo dnf install cmake webkit2gtk4-devel gtk3-devel sqlite-devel libcurl-devel gcc-c++ pkg-config gdk-pixbuf2-devel gdb valgrind strace inotify-tools
```

### Dependencies by Category

#### 🔧 Build Tools (Required)

```bash
sudo pacman -S cmake base-devel pkg-config
```

#### 📚 Main Libraries (Required)

```bash
# WebKit2GTK 4.1 (web rendering engine)
sudo pacman -S webkit2gtk

# GTK+3 (graphical interface)
sudo pacman -S gtk3

# SQLite3 (database)
sudo pacman -S sqlite

# cURL (HTTP requests)
sudo pacman -S curl
```

#### 🐞 Debug Tools (Recommended)

```bash
# GDB (debugger)
sudo pacman -S gdb

# Valgrind (memory leak detection)
sudo pacman -S valgrind

# strace (system call tracing)
sudo pacman -S strace
```

#### 🔄 Monitoring Tools (for `make watch`)

```bash
# inotify-tools (real-time file monitoring)
sudo pacman -S inotify-tools
# entr (alternative to inotify-tools, if preferred or if inotify-tools causes problems)
# sudo pacman -S entr
```

### Minimal Installation (Without Debug Tools)

If you only want to compile and launch the application:

```bash
sudo pacman -S cmake webkit2gtk gtk3 sqlite curl base-devel pkg-config
```

### Installation Verification

After installation, verify everything is in place:

```bash
# Check CMake
cmake --version

# Check main libraries
pkg-config --exists webkit2gtk-4.1 && echo "✅ WebKit2GTK: OK" || echo "❌ WebKit2GTK: MISSING"
pkg-config --exists sqlite3 && echo "✅ SQLite3: OK" || echo "❌ SQLite3: MISSING"
pkg-config --exists gtk+-3.0 && echo "✅ GTK+3: OK" || echo "❌ GTK+3: MISSING"

# Check debugging tools
command -v gdb >/dev/null 2>&1 && echo "✅ GDB: OK" || echo "⚠️  GDB: Not installed (optional)"
command -v valgrind >/dev/null 2>&1 && echo "✅ Valgrind: OK" || echo "⚠️  Valgrind: Not installed (optional)"
command -v strace >/dev/null 2>&1 && echo "✅ strace: OK" || echo "⚠️  strace: Not installed (optional)"

# Check monitoring tools
command -v inotifywait >/dev/null 2>&1 && echo "✅ inotifywait: OK" || echo "⚠️  inotifywait: Not installed (optional for watch)"
command -v entr >/dev/null 2>&1 && echo "✅ entr: OK" || echo "⚠️  entr: Not installed (optional for watch)"
```

### Using the Makefile to Check Dependencies

The Makefile includes a command to automatically check dependencies:

```bash
make check-deps
```
