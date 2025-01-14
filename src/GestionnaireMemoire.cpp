#include "GestionnaireMemoire.h"
#include <iostream>
#include <webkit2/webkit2.h>
#include <sys/sysinfo.h>
#include <thread>

void GestionnaireMemoire::surveillerUtilisationMemoire() {
    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        std::cout << "Mémoire utilisée : " 
                  << (info.totalram - info.freeram) / (1024 * 1024) << " Mo / "
                  << info.totalram / (1024 * 1024) << " Mo" << std::endl;
    } else {
        std::cerr << "Erreur : Impossible de récupérer l'utilisation mémoire." << std::endl;
    }
}

void GestionnaireMemoire::optimiserMemoire() {
    WebKitWebContext *context = webkit_web_context_get_default();
    if (context) {
        std::cout << "Optimisation mémoire : vidage du cache WebKit..." << std::endl;
        webkit_web_context_clear_cache(context);
    } else {
        std::cerr << "Erreur : Contexte WebKit non disponible pour optimisation." << std::endl;
    }
}


void GestionnaireMemoire::hibernerOnglet(WebKitWebView *onglet) {
    if (onglet && WEBKIT_IS_WEB_VIEW(onglet)) {
        std::cout << "Mise en veille de l'onglet pour économiser des ressources." << std::endl;
        webkit_web_view_stop_loading(onglet); // Arrête les requêtes réseau
        gtk_widget_hide(GTK_WIDGET(onglet));
    }
}

void GestionnaireMemoire::reactiverOnglet(WebKitWebView *onglet, const std::string &url) {
    if (onglet && WEBKIT_IS_WEB_VIEW(onglet)) {
        std::cout << "Réactivation de l'onglet avec URL : " << url << std::endl;
        webkit_web_view_load_uri(onglet, url.c_str());
        gtk_widget_show(GTK_WIDGET(onglet));
    }
}

void GestionnaireMemoire::viderCache() {
    std::cout << "Vidage du cache WebKit..." << std::endl;

    WebKitWebContext *context = webkit_web_context_get_default();
    if (context) {
        webkit_web_context_clear_cache(context);
        std::cout << "Cache WebKit vidé avec succès." << std::endl;
    } else {
        std::cerr << "Erreur : Impossible de vider le cache. Contexte WebKit non disponible." << std::endl;
    }
}