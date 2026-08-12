# Interfaces utilisateur WeedlyWeb

```
src/ui/
├── gtk/       # Backend principal — GTK3 + WebKit2GTK  → binaire WeedlyWeb
├── qt/        # Backend alternatif — Qt6 + WebEngine   → binaire WeedlyWebQt
├── android/   # Stub (futur)
├── cocoa/     # Stub macOS natif (futur)
└── win32/     # Stub Windows natif (futur)
```

Logique partagée (fichiers, JSON favoris, HTTP, DB, groupes d’onglets) :
`src/core/` + `include/` (+ `include/core/`).

## Compilation

```bash
cmake -B build -S . -DBUILD_UI_GTK=ON -DBUILD_UI_QT=ON
cmake --build build
```

```bash
make run-gtk   # ou make run
make run-qt
WEEDLYWEB_UI=qt ./scripts/run-weedlyweb.sh
```
