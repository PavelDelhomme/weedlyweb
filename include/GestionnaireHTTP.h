#ifndef GESTIONNAIREHTTP_H
#define GESTIONNAIREHTTP_H

#include <string>
#include <iostream>
#include <curl/curl.h>

class GestionnaireHTTP {
public:
    std::string recuperer(const std::string& url);
};

#endif
