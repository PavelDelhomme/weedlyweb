# 📊 WeedlyWeb - État du Projet

**Dernière mise à jour :** 2025-12-15  
**Branche active :** fusion_github_features  
**Version :** 1.0

---

## ✅ Fonctionnalités Implémentées

### Navigation de base
- ✅ Gestion des tabs (ajout, suppression, navigation)
- ✅ Barre d'URL fonctionnelle
- ✅ Barre de navigation (retour, avancer, recharger)
- ✅ Chargement et affichage des pages web

### Favoris
- ✅ Ajout de favorites
- ✅ Suppression de favorites
- ✅ Barre de favorites dédiée
- ✅ Gestionnaire de favorites avec interface
- ✅ Menu contextuel pour les favorites

### Gestion mémoire
- ✅ Surveillance de l'utilisation mémoire avec `MemoryManager`
- ✅ Optimisation de la mémoire (vidage du cache)
- ✅ Mise en veille des tabs inactifs
- ✅ Réactivation des tabs à partir de leur URL

### Sécurité HTTP
- ✅ Vérification des certificats SSL dans `HTTPManager`
- ✅ Affichage des erreurs de connexion HTTPS

---

## 🚧 En Cours de Développement

### Interface utilisateur
- ✅ Réorganisation complète de l'interface (barres en haut, zone web en bas)
- ✅ Système d'tabs simplifié avec button "+" simple
- ✅ Menu hamburger (trois barres) pour les options
- ✅ Design minimaliste et moderne avec CSS
- ✅ Barre de favorites améliorée (affichage limité à 10, button "⋯" pour le reste)
- ✅ Correction des erreurs de fermeture (GLib-GObject-CRITICAL)
- ✅ Fenêtre apparaît correctement dans la barre des tâches (XFCE/KDE/GNOME)

### Infrastructure
- ✅ Makefile complet avec commandes build, run, debug, valgrind
- ✅ Script d'installation automatique des dépendances (`install-deps.sh`)
- ✅ Documentation complète (`INSTALL_DEPENDENCIES.md`, `GUIDE_DEBUG.md`)
- ✅ Configuration GDB avec `.gdbinit` et commandes personnalisées
- ✅ Vérification automatique des dépendances (`make check-deps`)

---

## 📋 Fonctionnalités Planifiées

### Priorité Haute
- [ ] Page de paramètres utilisateur complète
- [ ] Bouton pour vider le cache manuellement
- [ ] Amélioration de la gestion des erreurs WebKit/GTK

### Priorité Moyenne
- [ ] Animations pour les transitions d'tabs
- [ ] Amélioration de l'UX de la barre d'tabs
- [ ] Système de logs structuré

### Priorité Basse
- [ ] Historique de navigation
- [ ] Mode sombre
- [ ] Accès aux cookies
- [ ] Informations serveur
- [ ] Support Tor
- [ ] Intégration similaire à BurpSuite
- [ ] Gestion manuelle des requêtes
- [ ] Analyse des éléments chargés
- [ ] Indexation automatique des pages

---

## 🐛 Bugs Connus

- [ ] Erreur "WebView invalide" dans les logs (occasionnel, non bloquant)
- [ ] Avertissement "Failed to create GBM buffer" au démarrage (non bloquant, lié au driver graphique)
- [x] ~~Erreurs GLib-GObject-CRITICAL à la fermeture~~ (Corrigé)
- [x] ~~Fenêtre n'apparaissait pas dans la barre des tâches~~ (Corrigé)
- [x] ~~Barre de favorites mal affichée~~ (Corrigé)

---

## 🔧 Améliorations Techniques Récentes

### 2025-12-15
- ✅ Réorganisation complète de l'interface utilisateur
  - Barres (tabs, navigation, favorites) en haut
  - Zone de rendu web en bas (expandable)
- ✅ Système d'tabs simplifié
  - Bouton "+" simple pour ajouter un onglet
  - Onglets avec titre et button fermer (×)
  - Suppression du button "Changer Groupe" visible au démarrage
- ✅ Menu hamburger (trois barres) pour les options
  - Gestionnaire de Favoris
  - Paramètres
  - À propos
  - Quitter
- ✅ Design minimaliste et moderne
  - CSS personnalisé pour tous les composants
  - Barre de favorites avec affichage limité (10 favorites max)
  - Bouton "⋯" pour les favorites restants
  - Effets hover/active sur les boutons
- ✅ Correction des erreurs de fermeture
  - Nettoyage propre des signaux GTK dans le destructeur
  - Plus d'erreurs GLib-GObject-CRITICAL
- ✅ Configuration pour la barre des tâches
  - Fenêtre apparaît correctement dans XFCE/KDE/GNOME
  - Propriétés X11 correctement définies
- ✅ Documentation et outils de développement
  - `INSTALL_DEPENDENCIES.md` : Guide complet d'installation
  - `GUIDE_DEBUG.md` : Guide d'utilisation de GDB
  - `install-deps.sh` : Script d'installation automatique
  - `.gdbinit` : Configuration GDB avec commandes personnalisées
  - `make check-deps` : Vérification automatique des dépendances

### 2025-01-24
- Création de la branche `dev` à partir de `origin/dev_avant_modif_global_favoris`
- Ajout d'un Makefile complet pour la gestion du projet
- Transformation de `todos.md` en `STATUS.md`

### 2025-01-16
- Gestion des favorites avec boutons et barre de favorites
- Amélioration de la gestion des tabs

---

## 📈 Métriques du Projet

- **Langage :** C++17
- **Framework GUI :** GTK+3
- **Moteur de rendu :** WebKit2GTK 4.1
- **Gestionnaire de build :** CMake + Makefile
- **Dépendances principales :**
  - WebKit2GTK 4.1
  - GTK+3
  - cURL
  - SQLite3
  - nlohmann/json
- **Outils de développement :**
  - GDB (débogage)
  - Valgrind (détection de fuites mémoire)
  - strace (traçage système)

---

## 🎯 Objectifs à Court Terme

1. Résoudre les bugs critiques liés aux tabs
2. Nettoyer le code commenté
3. Finaliser la migration vers Makefile
4. Améliorer la gestion des erreurs

---

## 📝 Notes

- Le projet utilise une architecture modulaire avec des gestionnaires séparés
- La gestion mémoire est optimisée pour les applications longues sessions
- Le système de favorites est fonctionnel mais peut être amélioré au niveau UX

