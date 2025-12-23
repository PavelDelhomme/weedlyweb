#ifndef MEMORYMANAGER_H
#define MEMORYMANAGER_H

#include <webkit2/webkit2.h>
#include <string>

class MemoryManager {
public:
    void surveillerUtilisationMemoire();
    void optimiserMemoire();
    void hibernerOnglet(WebKitWebView *onglet);
    void reactiverOnglet(WebKitWebView *onglet, const std::string &url);
    void viderCache();
    
    // Nouvelles méthodes pour l'analyse détaillée
    void afficherStatistiquesMemoire() const;
    size_t obtenirMemoireUtilisee() const;  // En KB
    size_t obtenirMemoireVirtuelle() const; // En KB
};

#endif
