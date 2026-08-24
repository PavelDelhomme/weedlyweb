# Affichage multi-écrans — WeedlyWeb

WeedlyWeb cible les setups **multi-moniteurs** (KDE, GNOME, X11, Wayland).

## Au lancement (`make run`)

1. Détection du **moniteur sous le curseur** (pas le bureau virtuel étendu)
2. Placement de la fenêtre sur ce moniteur
3. **Maximisation** sur la zone de travail (`workarea`) de cet écran

Lanceur : `scripts/run-weedlyweb.sh` — essaie `wayland` puis `x11` selon la session.

```bash
make run          # GTK (défaut)
WEEDLYWEB_UI=qt make run-qt   # Qt6
```

Variables utiles :

```bash
WEEDLYWEB_GDK_BACKEND=x11 ./scripts/run-weedlyweb.sh
WEEDLYWEB_ALLOW_GPU=1 ./scripts/run-weedlyweb.sh
```

## Plein écran (F11)

- **Avant** : `gtk_window_fullscreen()` pouvait couvrir plusieurs écrans (bureau virtuel)
- **Maintenant** : `gtk_window_fullscreen_on_monitor()` sur le moniteur où la fenêtre est affichée

Appuyer à nouveau sur **F11** pour quitter le plein écran.

## Problèmes connus

| Symptôme | Piste |
|----------|--------|
| Fenêtre trop large / centrée entre 2 écrans | Relancer avec la souris sur l'écran voulu ; vérifier que `GTK_WIN_POS_CENTER` n'est pas forcé |
| F11 couvre tous les écrans | Mettre à jour WeedlyWeb (correctif `fullscreen_on_monitor`) |
| Qt sur mauvais écran | `make run-qt` utilise `QApplication::screenAt(QCursor::pos())` |

Voir aussi [COMPATIBILITY.md](COMPATIBILITY.md).
