# 📋 WeedlyWeb TODO List

## 🟢 Fonctionnalités principales (Haute priorité)
- [ ] Gestion des onglets :
  - [ ] Ajouter un nouvel onglet.
  - [ ] Supprimer un onglet.
  - [ ] Naviguer entre les onglets.
- [ ] Barre d'URL :
  - [ ] Saisir une URL et charger la page.
  - [ ] Afficher l'URL actuelle.
- [ ] Barre de navigation :
  - [ ] Bouton retour.
  - [ ] Bouton avancer.
  - [ ] Bouton recharger.

## 🟡 Fonctionnalités secondaires (Moyenne priorité)
- [ ] Favoris :
  - [ ] Ajouter un favori.
  - [ ] Supprimer un favori.
  - [ ] Afficher les favoris dans une barre dédiée.
- [ ] Paramètres utilisateur :
  - [ ] Créer une page pour modifier les paramètres.
- [ ] Gestion mémoire :
  - [ ] Implémenter la fonction `optimiserMemoire` dans `GestionnaireMemoire`.
  - [ ] Ajouter un bouton pour vider le cache.

## 🔵 Fonctionnalités UX/UI
- [ ] Chargement des titres d'onglets :
  - [ ] Afficher le titre de la page active dans l'onglet.
- [ ] Bouton "+" dans la barre d'onglets :
  - [ ] Permettre l'ajout rapide d'onglets.
- [ ] Ajout des animations :
  - [ ] Ajouter des transitions pour ouvrir/fermer des onglets.

## ⚙️ Fonctionnalités techniques
- [ ] Suivi des erreurs :
  - [ ] Afficher les erreurs WebKit et GTK dans un log.
- [ ] Optimisation des performances :
  - [ ] Surveiller l'utilisation CPU/mémoire avec `GestionnaireMemoire`.
- [x] Surveiller l'utilisation mémoire avec `GestionnaireMemoire`.
- [x] Optimiser la mémoire en vidant le cache.
- [x] Implémenter la mise en veille des onglets inactifs.
- [x] Réactiver les onglets à partir de leur URL.


## Fonctionnalités liées au HTTP
- [x] Vérifier les certificats SSL dans `GestionnaireHTTP`.
- [x] Afficher les erreurs de connexion HTTPS.

---

## 🛠️ Bugs à corriger
- [ ] Erreur "WebView invalide" dans les logs.
- [ ] Titre de l'onglet restant bloqué sur "Chargement...".
- [ ] Bouton "+" mal placé après ajout d'un onglet.
- [ ] Résoudre les crashs sur suppression d'onglets pendant le chargement.
- [ ] Corriger les cas où les titres des onglets ne se mettent pas à jour.

---

## 📦 Fonctionnalités futures
- [ ] Historique de navigation :
  - [ ] Suivre les pages visitées et permettre de les consulter.
- [ ] Mode sombre :
  - [ ] Ajouter un thème sombre pour l'interface.
