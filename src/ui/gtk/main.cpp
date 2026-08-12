#include <cstdlib>
#include <iostream>
#include <gtk/gtk.h>
#include "browser/Browser.h"

namespace {

bool hasGraphicalDisplay() {
    const char* display = std::getenv("DISPLAY");
    const char* wayland = std::getenv("WAYLAND_DISPLAY");
    return (display && display[0] != '\0') || (wayland && wayland[0] != '\0');
}

void printDisplayHint() {
    std::cerr << "WeedlyWeb: aucun serveur d'affichage détecté.\n"
              << "  - Session X11     : DISPLAY doit être défini (ex. :0)\n"
              << "  - Session Wayland : WAYLAND_DISPLAY doit être défini\n"
              << "  - Forcer backend  : GDK_BACKEND=x11 ou GDK_BACKEND=wayland\n"
              << "  - Lanceur         : ./scripts/run-weedlyweb.sh\n"
              << "  Voir docs/COMPATIBILITY.md\n";
}

} // namespace

int main(int argc, char* argv[]) {
    if (!hasGraphicalDisplay()) {
        printDisplayHint();
        return 1;
    }

    // gtk_init_check évite un abort brutal si le display est invalide
    if (!gtk_init_check(&argc, &argv)) {
        std::cerr << "WeedlyWeb: échec d'initialisation GTK (display inaccessible).\n";
        printDisplayHint();
        const char* gdkBackend = std::getenv("GDK_BACKEND");
        if (gdkBackend) {
            std::cerr << "  GDK_BACKEND actuel = " << gdkBackend << "\n";
        }
        return 1;
    }

    // Thème sombre préféré (respecte le thème système si disponible)
    if (GtkSettings* settings = gtk_settings_get_default()) {
        g_object_set(settings, "gtk-application-prefer-dark-theme", TRUE, NULL);
    }

    Browser navigateur;

    gtk_main();
    return 0;
}
