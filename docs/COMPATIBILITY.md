# Compatibilité plateformes — WeedlyWeb

WeedlyWeb (cible GTK + WebKit2GTK) est conçu pour **Linux desktop** : X11 et Wayland, tous les grands environnements.

## Backends UI

| Backend | Chemin | Binaire | Lancement |
|---|---|---|---|
| GTK + WebKit2GTK | `src/ui/gtk` | `WeedlyWeb` | `make run-gtk` |
| Qt6 + WebEngine | `src/ui/qt` | `WeedlyWebQt` | `make run-qt` / `WEEDLYWEB_UI=qt ./scripts/run-weedlyweb.sh` |
| Core partagé | `src/core` | `libWeedlyWebCore.a` | — |
| Android / Cocoa / Win32 | stubs | — | futurs |

CMake : `-DBUILD_UI_GTK=ON -DBUILD_UI_QT=ON` (les deux en parallèle).

## Linux (support principal)

| Environnement | Affichage | Statut |
|---|---|---|
| GNOME | Wayland / X11 | OK via `scripts/run-weedlyweb.sh` |
| KDE Plasma | Wayland / X11 | OK |
| XFCE / Xubuntu | X11 (Wayland partiel) | OK |
| LXQt / Lubuntu | X11 | OK |
| MATE | X11 | OK |
| Cinnamon | X11 / Wayland | OK |
| i3 / sway | X11 / Wayland | OK |
| dwm / awesome / openbox | X11 | OK (`NO_AT_BRIDGE=1`) |
| Hyprland | Wayland | OK (fallback XWayland si besoin) |

### Lancement recommandé

```bash
make run
# ou
./scripts/run-weedlyweb.sh
```

Le lanceur essaie automatiquement les backends GDK dans un ordre adapté à la session :

1. session Wayland → `wayland` puis `x11` (XWayland)
2. session X11 → `x11`
3. `GDK_BACKEND` forcé par l’utilisateur → respecté tel quel

Variables utiles :

```bash
WEEDLYWEB_GDK_BACKEND=x11 ./scripts/run-weedlyweb.sh      # forcer X11 uniquement
WEEDLYWEB_GDK_BACKEND=wayland ./scripts/run-weedlyweb.sh  # forcer Wayland uniquement
WEEDLYWEB_ALLOW_GPU=1 ./scripts/run-weedlyweb.sh           # autoriser GPU (désactive soft GL)
WEEDLYWEB_NONINTERACTIVE=1 ./scripts/install-deps.sh      # deps sans questions
```

### Installation système (menu applications)

```bash
sudo cmake --install build
# ou copie manuelle :
sudo install -Dm755 scripts/weedlyweb /usr/local/bin/weedlyweb
sudo install -Dm755 scripts/run-weedlyweb.sh /usr/local/lib/weedlyweb/run-weedlyweb.sh
sudo install -Dm755 build/WeedlyWeb /usr/local/bin/WeedlyWeb
sudo install -Dm644 assets/desktop/weedlyweb.desktop /usr/local/share/applications/weedlyweb.desktop
sudo install -Dm644 assets/icons/weedlyweb.png /usr/local/share/icons/hicolor/256x256/apps/weedlyweb.png
update-desktop-database ~/.local/share/applications 2>/dev/null || true
```

Compatible avec les menus de **KDE**, **GNOME**, **XFCE**, **LXQt (Lubuntu)**, **MATE (Ubuntu MATE / Xubuntu-like)**.

## Windows

Pas de binaire natif Win32 pour l’instant (GTK/WebKit2GTK). Options :

1. **WSL2 + WSLg** (recommandé) : installer les deps Linux dans WSL, lancer `./scripts/run-weedlyweb.sh`
2. **MSYS2** (expérimental) : paquets `mingw-w64-x86_64-gtk3` / webkit — non validé en CI

## macOS

Expérimental via Homebrew (`gtk+3`, `webkitgtk`) : le code source compile en théorie, mais le packaging `.app` n’est pas fourni. Priorité = Linux.

## Dépendances multi-distro

```bash
./scripts/install-deps.sh
```

Couvre Arch/Manjaro, Debian/Ubuntu/Lubuntu/Xubuntu, Fedora, openSUSE.
