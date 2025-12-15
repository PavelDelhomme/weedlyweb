# Configuration GDB pour WeedlyWeb
# Ce fichier est chargé automatiquement par GDB

# Désactiver la pagination pour un affichage continu
set pagination off

# Afficher les valeurs des variables en hexadécimal
set print elements 200
set print null-stop on

# Afficher les pointeurs en hexadécimal
set print address on

# Configuration pour les crashes
# Afficher automatiquement la stack trace en cas de signal
define hook-stop
    bt
end

# Commandes utiles pour déboguer WeedlyWeb
define weedlyweb-help
    echo Commandes spécifiques à WeedlyWeb:\n
    echo   weedlyweb-bt    - Afficher la stack trace complète\n
    echo   weedlyweb-info  - Afficher les informations sur les widgets GTK\n
    echo   weedlyweb-break - Placer des breakpoints aux points critiques\n
end

# Breakpoints aux points critiques
define weedlyweb-break
    break Navigateur::construireInterface
    break Navigateur::ajouterNouvelOnglet
    break MoteurRendu::afficherPage
    echo Breakpoints placés aux points critiques\n
end

# Afficher les informations sur les widgets GTK
define weedlyweb-info
    if $argc == 1
        print *$arg0
    else
        echo Usage: weedlyweb-info <pointeur_widget>\n
    end
end

# Afficher la stack trace avec plus de détails
define weedlyweb-bt
    bt full
    info registers
    info locals
end

document weedlyweb-help
    Affiche l'aide pour les commandes spécifiques à WeedlyWeb
end

document weedlyweb-break
    Place des breakpoints aux points critiques de l'application
end

document weedlyweb-info
    Affiche les informations sur un widget GTK
    Usage: weedlyweb-info <pointeur_widget>
end

document weedlyweb-bt
    Affiche une stack trace complète avec les registres et variables locales
end

