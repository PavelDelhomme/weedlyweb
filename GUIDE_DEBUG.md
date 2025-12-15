# 🐞 Guide de Débogage avec GDB

Ce guide explique comment utiliser GDB pour déboguer WeedlyWeb.

## Commandes Makefile

### Mode interactif (recommandé)

```bash
make debug
```

Cette commande :
- Compile le projet en mode debug (avec symboles)
- Lance GDB en mode interactif
- Vous permet de contrôler l'exécution pas à pas

### Mode automatique

```bash
make debug-auto
```

Cette commande :
- Compile le projet en mode debug
- Lance l'application automatiquement
- Affiche la stack trace si un crash se produit
- Quitte automatiquement

## Commandes GDB de base

Une fois dans GDB, voici les commandes les plus utiles :

### Lancer l'application

```
(gdb) run
```

### Arrêter l'exécution

Appuyez sur `Ctrl+C` pour interrompre l'exécution.

### Afficher la stack trace (backtrace)

```
(gdb) bt
```

Pour une stack trace complète avec les variables locales :

```
(gdb) bt full
```

### Placer un breakpoint

```
(gdb) break Navigateur::construireInterface
(gdb) break src/Navigateur.cpp:343
```

### Continuer l'exécution

```
(gdb) continue
```

ou simplement :

```
(gdb) c
```

### Exécuter ligne par ligne

```
(gdb) next    # Passe à la ligne suivante (sans entrer dans les fonctions)
(gdb) step    # Passe à la ligne suivante (entre dans les fonctions)
```

### Afficher une variable

```
(gdb) print fenetre
(gdb) print *navigateur
```

### Quitter GDB

```
(gdb) quit
```

ou simplement :

```
(gdb) q
```

## Commandes personnalisées WeedlyWeb

Le fichier `.gdbinit` définit des commandes personnalisées :

### Afficher l'aide

```
(gdb) weedlyweb-help
```

### Placer des breakpoints aux points critiques

```
(gdb) weedlyweb-break
```

Cette commande place automatiquement des breakpoints aux fonctions importantes :
- `Navigateur::construireInterface`
- `Navigateur::ajouterNouvelOnglet`
- `MoteurRendu::afficherPage`

### Afficher une stack trace complète

```
(gdb) weedlyweb-bt
```

Affiche la stack trace avec les registres et variables locales.

### Afficher les informations sur un widget GTK

```
(gdb) weedlyweb-info <pointeur_widget>
```

## Exemple de session de débogage

```bash
# 1. Compiler et lancer GDB
make debug

# 2. Dans GDB, placer des breakpoints
(gdb) weedlyweb-break

# 3. Lancer l'application
(gdb) run

# 4. Si un crash se produit, afficher la stack trace
(gdb) bt full

# 5. Examiner les variables
(gdb) print fenetre
(gdb) print *navigateur

# 6. Quitter
(gdb) quit
```

## Déboguer un crash (Error 139 - SIGSEGV)

Si vous obtenez une erreur 139 (segmentation fault) :

```bash
# 1. Compiler en mode debug
make build-debug

# 2. Lancer avec GDB
make debug

# 3. Dans GDB
(gdb) run

# 4. Quand le crash se produit, GDB s'arrête automatiquement
# Affichez la stack trace
(gdb) bt full

# 5. Examinez les variables autour de la ligne du crash
(gdb) list
(gdb) print <variable>
```

## Conseils

1. **Toujours compiler en mode debug** avant d'utiliser GDB :
   ```bash
   make build-debug
   ```

2. **Utiliser les breakpoints** pour arrêter l'exécution à des points précis

3. **Examiner les pointeurs** : Si un pointeur est `NULL`, c'est souvent la cause d'un crash

4. **Vérifier les widgets GTK** : Utilisez `weedlyweb-info` pour examiner les widgets

5. **Utiliser `bt full`** pour voir toutes les variables locales dans la stack trace

## Ressources

- [Documentation officielle GDB](https://www.gnu.org/software/gdb/documentation/)
- [GDB Cheat Sheet](https://darkdust.net/files/GDB%20Cheat%20Sheet.pdf)

