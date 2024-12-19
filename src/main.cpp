#include <iostream>
#include <gtk/gtk.h>
#include "Navigateur.h"

int main(int argc, char* argv[]) {
    gtk_init(&argc, &argv); // Initialisation de GTK

    Navigateur navigateur;
    navigateur.lancer();

    gtk_main(); // Boucle principale de l'application
    return 0;
}
