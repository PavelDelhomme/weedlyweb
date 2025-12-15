# 📦 Dependencies Installation for WeedlyWeb

## System: Manjaro Linux (Arch Linux)

### Automatic Installation (Recommended)

An automatic installation script is available:

```bash
./install-deps.sh
```

This script will guide you through the installation and offer to install debugging tools.

### Full Installation in One Command

To install all required dependencies (compilation + debugging):

```bash
sudo pacman -S cmake webkit2gtk gtk3 sqlite curl base-devel pkg-config gdb valgrind strace
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
