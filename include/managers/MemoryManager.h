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

};

#endif
