#include <iostream>
#include <gtk/gtk.h>
#include "browser/Browser.h"

int main(int argc, char* argv[]) {
    gtk_init(&argc, &argv); // Initialisation de GTK

    // Configurer GTK pour utiliser le mode sombre par défaut (utilise le thème système)
    GtkSettings* settings = gtk_settings_get_default();
    g_object_set(settings, "gtk-application-prefer-dark-theme", TRUE, NULL);

    Browser navigateur;

    gtk_main(); // Boucle principale de l'application
    return 0;
}
