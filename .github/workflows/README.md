# Configuration des workflows GitHub

## Notifications par email

Le workflow `notify-on-push.yml` envoie des notifications par email à chaque push sur les branches `main`, `dev` et `feat/**`.

### Configuration requise

Pour activer les notifications par email, vous devez configurer les secrets suivants dans GitHub :

1. Allez dans **Settings** > **Secrets and variables** > **Actions**
2. Ajoutez les secrets suivants :

   - `EMAIL_USERNAME` : Votre adresse email Gmail (ou serveur SMTP)
   - `EMAIL_PASSWORD` : Mot de passe de l'application Gmail (ou mot de passe SMTP)
   - `EMAIL_TO` : Adresse email de destination pour les notifications

### Configuration Gmail

Si vous utilisez Gmail, vous devez créer un "Mot de passe d'application" :

1. Allez sur https://myaccount.google.com/apppasswords
2. Créez un nouveau mot de passe d'application
3. Utilisez ce mot de passe dans le secret `EMAIL_PASSWORD`

### Autres serveurs SMTP

Si vous utilisez un autre serveur SMTP, modifiez les paramètres dans `.github/workflows/notify-on-push.yml` :

- `server_address` : Adresse du serveur SMTP
- `server_port` : Port SMTP (465 pour SSL, 587 pour TLS)

## Build et test

Le workflow `build-and-test.yml` compile automatiquement le projet à chaque push pour vérifier que tout fonctionne correctement.

