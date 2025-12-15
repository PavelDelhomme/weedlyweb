#ifndef SCRIPTENGINE_H
#define SCRIPTENGINE_H

#include <string>
#include <webkit2/webkit2.h>

class ScriptEngine {
public:
    // Exécute un script JavaScript dans la vue Web spécifiée
    void executerScript(WebKitWebView* webView, const std::string& script);

    // Exécute un script JavaScript et retourne sa valeur en tant que chaîne
    std::string obtenirValeur(WebKitWebView* webView, const std::string& script);

    // Configure des écouteurs d'événements JavaScript (e.g., pour des clics ou mouvements)
    void ajouterEvenement(WebKitWebView* webView, const std::string& evenement, const std::string& callback);
};

#endif
