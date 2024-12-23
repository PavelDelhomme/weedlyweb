#ifndef GESTIONNAIREFICHIERS_H
#define GESTIONNAIREFICHIERS_H

#include <string>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

class GestionnaireFichiers {
public:
    static std::string obtenirCheminRacine();
    // Méthodes pour obtenir les chemins absolus
    static std::string obtenirCheminAbsolu(const std::string& cheminRelatif);
    static std::string cheminConfigJSON();
    static std::string cheminFavorisJSON();
    static std::string cheminParametresHTML();
    static std::string cheminStylesCSS();

    // Méthodes pour lire et écrire des fichiers JSON
    static nlohmann::json lireJSON(const std::string& chemin);
    static void ecrireJSON(const std::string& chemin, const nlohmann::json& contenu);
};

#endif
