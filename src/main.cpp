#include <iostream>
#include <gtk/gtk.h>
#include "browser/Browser.h"

int main(int argc, char* argv[]) {
    gtk_init(&argc, &argv); // Initialisation de GTK

    Browser navigateur;

    gtk_main(); // Boucle principale de l'application
    return 0;
}
