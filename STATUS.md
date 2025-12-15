# 📊 WeedlyWeb - État du Projet

**Dernière mise à jour :** 2025-01-24  
**Branche active :** dev  
**Version :** 1.0

---

## ✅ Fonctionnalités Implémentées

### Navigation de base
- ✅ Gestion des onglets (ajout, suppression, navigation)
- ✅ Barre d'URL fonctionnelle
- ✅ Barre de navigation (retour, avancer, recharger)
- ✅ Chargement et affichage des pages web

### Favoris
- ✅ Ajout de favoris
- ✅ Suppression de favoris
- ✅ Barre de favoris dédiée
- ✅ Gestionnaire de favoris avec interface
- ✅ Menu contextuel pour les favoris

### Gestion mémoire
- ✅ Surveillance de l'utilisation mémoire avec `GestionnaireMemoire`
- ✅ Optimisation de la mémoire (vidage du cache)
- ✅ Mise en veille des onglets inactifs
- ✅ Réactivation des onglets à partir de leur URL

### Sécurité HTTP
- ✅ Vérification des certificats SSL dans `GestionnaireHTTP`
- ✅ Affichage des erreurs de connexion HTTPS

---

## 🚧 En Cours de Développement

### Interface utilisateur
- 🔄 Amélioration de la gestion des titres d'onglets
- 🔄 Optimisation de l'affichage de la barre de favoris
- 🔄 Refactoring du code commenté dans `Navigateur.cpp` et `Navigateur.h`

### Infrastructure
- 🔄 Migration vers Makefile (en cours)
- 🔄 Amélioration de la structure du projet

---

## 📋 Fonctionnalités Planifiées

### Priorité Haute
- [ ] Page de paramètres utilisateur complète
- [ ] Bouton pour vider le cache manuellement
- [ ] Amélioration de la gestion des erreurs WebKit/GTK

### Priorité Moyenne
- [ ] Animations pour les transitions d'onglets
- [ ] Amélioration de l'UX de la barre d'onglets
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

- [ ] Erreur "WebView invalide" dans les logs (occasionnel)
- [ ] Titre de l'onglet peut rester bloqué sur "Chargement..." dans certains cas
- [ ] Bouton "+" peut être mal placé après ajout d'un onglet
- [ ] Crashs possibles lors de la suppression d'onglets pendant le chargement
- [ ] Titres des onglets ne se mettent pas toujours à jour correctement

---

## 🔧 Améliorations Techniques Récentes

### 2025-01-24
- Création de la branche `dev` à partir de `origin/dev_avant_modif_global_favoris`
- Ajout d'un Makefile complet pour la gestion du projet
- Transformation de `todos.md` en `STATUS.md`

### 2025-01-16
- Gestion des favoris avec boutons et barre de favoris
- Amélioration de la gestion des onglets

---

## 📈 Métriques du Projet

- **Langage :** C++17
- **Framework GUI :** GTK+3
- **Moteur de rendu :** WebKit2GTK 4.1
- **Gestionnaire de build :** CMake
- **Dépendances principales :**
  - WebKit2GTK 4.1
  - GTK+3
  - cURL
  - nlohmann/json

---

## 🎯 Objectifs à Court Terme

1. Résoudre les bugs critiques liés aux onglets
2. Nettoyer le code commenté
3. Finaliser la migration vers Makefile
4. Améliorer la gestion des erreurs

---

## 📝 Notes

- Le projet utilise une architecture modulaire avec des gestionnaires séparés
- La gestion mémoire est optimisée pour les applications longues sessions
- Le système de favoris est fonctionnel mais peut être amélioré au niveau UX

