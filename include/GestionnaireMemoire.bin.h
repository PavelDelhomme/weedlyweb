#ifndef GESTIONNAIREMEMOIRE_H
#define GESTIONNAIREMEMOIRE_H

#include <webkit2/webkit2.h>
#include <string>

class GestionnaireMemoire {
public:
    void surveillerUtilisationMemoire();
    void optimiserMemoire();
    void hibernerOnglet(WebKitWebView *onglet);
    void reactiverOnglet(WebKitWebView *onglet, const std::string &url);
    void viderCache();

};

#endif
