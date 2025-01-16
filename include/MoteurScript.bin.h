#ifndef MOTEURSCRIPT_H
#define MOTEURSCRIPT_H

#include <string>
#include <webkit2/webkit2.h>

class MoteurScript {
public:
    // Exécute un script JavaScript dans la vue Web spécifiée
    void executerScript(WebKitWebView* vueWeb, const std::string& script);

    // Exécute un script JavaScript et retourne sa valeur en tant que chaîne
    std::string obtenirValeur(WebKitWebView* vueWeb, const std::string& script);

    // Configure des écouteurs d'événements JavaScript (e.g., pour des clics ou mouvements)
    void ajouterEvenement(WebKitWebView* vueWeb, const std::string& evenement, const std::string& callback);
};

#endif
