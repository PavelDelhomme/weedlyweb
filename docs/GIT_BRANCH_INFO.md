# 📋 Informations sur les branches Git

## Branche actuelle
- **Locale :** `feat/fusion_github_features`
- **Remote :** `origin/feat/fusion_github_features` (à créer)

## Branche de sauvegarde
Une branche de sauvegarde a été créée pour protéger votre travail :
- **Nom :** `backup-fusion_github_features-YYYYMMDD-HHMMSS`
- **But :** Permet de revenir en arrière si nécessaire

## Commandes utiles

### Voir toutes les branches
```bash
git branch -a
```

### Revenir à la branche de sauvegarde si nécessaire
```bash
git checkout backup-fusion_github_features-YYYYMMDD-HHMMSS
```

### Pousser la branche (si SSH n'est pas configuré)
```bash
# Option 1 : Avec authentification HTTPS
git push -u origin feat/fusion_github_features
# Entrer votre username GitHub et un token comme mot de passe

# Option 2 : Configurer SSH d'abord
ssh-keygen -t ed25519 -C "votre-email@example.com"
# Puis ajouter la clé publique à GitHub (Settings > SSH keys)
git push -u origin feat/fusion_github_features
```

### Merger dans main plus tard
```bash
# Depuis la branche feat/fusion_github_features
git checkout main
git pull origin main
git merge feat/fusion_github_features
git push origin main
```

