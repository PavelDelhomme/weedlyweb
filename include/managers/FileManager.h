#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <string>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

class FileManager {
public:
    static std::string obtenirCheminRacine();
    // Méthodes pour obtenir les chemins absolus
    static std::string obtenirCheminAbsolu(const std::string& cheminRelatif);
    static std::string configJSONPath();
    static std::string favoritesJSONPath();
    static std::string historyJSONPath();
    static std::string webkitDataDirectory();
    static std::string webkitCacheDirectory();
    static std::string cookiesDatabasePath();
    static std::string cheminParametresHTML();
    static std::string cheminHelpHTML();
    static std::string cheminStylesCSS();

    // Méthodes pour lire et écrire des fichiers JSON
    static nlohmann::json readJSON(const std::string& chemin);
    static void writeJSON(const std::string& chemin, const nlohmann::json& contenu);
};

#endif
