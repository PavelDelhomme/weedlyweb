# 📊 WeedlyWeb — État du projet

**Dernière mise à jour :** 2026-08-24  
**Branche active :** `hotfix/err-failed-gpu`  
**Version :** 1.x (GTK principal, Qt6 expérimental)

---

## ✅ Fonctionnalités implémentées

### Navigation
- Onglets (ajout, fermeture, duplication, groupes)
- Barre d'URL avec autocomplétion
- Barre de navigation (retour, avancer, recharger)
- WebKit2GTK 4.1 (GTK) / Qt WebEngine (Qt6)

### Onglets avancés (GTK)
- Menu contextuel : fermer, dupliquer, renommer, titre verrouillé
- Épingler / désépingler (molette ne ferme pas les épinglés)
- Favoris depuis l'onglet (ajouter / modifier / supprimer selon l'URL)
- Déplacer vers un groupe

### Favoris
- JSON hiérarchique + barre dédiée
- Gestionnaire avec dossiers
- Recherche par URL (`FavoritesJson::findByUrlRecursive`)

### Affichage et UX
- Mode sombre des pages (`PageEnhancements`)
- Mode lecture
- Multi-écrans : lancement sur moniteur sous curseur, F11 sur un seul écran
- Raccourcis globaux (key snooper GTK) : Ctrl+T/W/D, F11, etc.
- Palette de commandes (Ctrl+Shift+C)

### Infrastructure
- Architecture `src/core` + `src/ui/{gtk,qt}`
- CMake dual `BUILD_UI_GTK` / `BUILD_UI_QT`
- Lanceur `scripts/run-weedlyweb.sh` (Wayland / X11)
- Makefile : `run-gtk`, `run-qt`, `stop`, etc.

### Mémoire et sécurité
- `MemoryManager`, cache WebKit
- Vérification SSL (`HTTPManager`)
- Intercepteur de requêtes, cookies persistants

---

## 🚧 En cours / partiel

| Élément | GTK | Qt6 |
|---------|-----|-----|
| Navigation de base | ✅ | ✅ squelette |
| Favoris / groupes | ✅ | partiel |
| Menu onglet / épinglage | ✅ | ❌ |
| Mode sombre / lecture | ✅ | ❌ |
| Multi-écran | ✅ | ✅ placement initial |

---

## 📋 Planifié

### Priorité haute
- [ ] Parité fonctionnelle Qt6 avec GTK
- [ ] Page paramètres complète
- [ ] Bouton vider le cache manuellement

### Priorité moyenne
- [ ] Historique de navigation UI
- [ ] Animations onglets
- [ ] Logs structurés

### Priorité basse
- [ ] Support Tor, analyse CVE avancée
- [ ] Backends Android / Cocoa / Win32 (stubs présents)

---

## 🐛 Bugs connus

- Avertissement « Failed to create GBM buffer » (driver GPU, souvent non bloquant)
- `gtk_key_snooper_install` déprécié (GTK 4 migration future)
- Qt : fonctionnalités inférieures au backend GTK

### Corrigés récemment
- [x] F11 couvrait tout le bureau virtuel multi-écrans
- [x] Fenêtre centrée entre plusieurs moniteurs au lancement
- [x] Raccourcis ignorés quand le focus est dans WebKit
- [x] GLib-GObject-CRITICAL à la fermeture

---

## 🔧 Améliorations récentes

### 2026-08-24
- Menu contextuel onglets + épinglage + favoris depuis l'onglet
- Plein écran par moniteur (`fullscreen_on_monitor`)
- Raccourcis clavier via key snooper
- Placement fenêtre multi-écrans (GTK + Qt)
- Documentation : README, QUICKSTART, KEYBOARD_SHORTCUTS, DISPLAY_AND_MONITORS

### 2026-08 (architecture)
- Refactor `src/core` + `src/ui/gtk|qt`
- Modes sombre et lecture (`PageEnhancements`)
- Lanceur multi-backend et `.desktop`

---

## 📈 Métriques

| | |
|---|---|
| Langage | C++17 |
| GUI | GTK+3 (principal), Qt6 (alternatif) |
| Rendu | WebKit2GTK 4.1, Qt WebEngine |
| Build | CMake + Makefile |
| Données | SQLite3, JSON (favoris, config, session) |

---

## 📝 Notes

- Backend **GTK** = référence pour les nouvelles fonctionnalités
- Voir [COMPATIBILITY.md](COMPATIBILITY.md), [MIGRATION_QT.md](MIGRATION_QT.md)
