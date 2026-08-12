#include "managers/FileManager.h"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace {

std::string xdgDataHome() {
    if (const char* xdg = std::getenv("XDG_DATA_HOME"); xdg && xdg[0] != '\0') {
        return xdg;
    }
    if (const char* home = std::getenv("HOME"); home && home[0] != '\0') {
        return std::string(home) + "/.local/share";
    }
    return "/tmp";
}

std::string xdgCacheHome() {
    if (const char* xdg = std::getenv("XDG_CACHE_HOME"); xdg && xdg[0] != '\0') {
        return xdg;
    }
    if (const char* home = std::getenv("HOME"); home && home[0] != '\0') {
        return std::string(home) + "/.cache";
    }
    return "/tmp";
}

} // namespace

std::string FileManager::obtenirCheminRacine() {
    auto current_path = std::filesystem::current_path();
    if (current_path.filename() == "build" || current_path.filename() == "bin") {
        return current_path.parent_path().string();
    }
    // build/ui-qt, build/gtk, etc.
    if (current_path.parent_path().filename() == "build") {
        return current_path.parent_path().parent_path().string();
    }
    return current_path.string();
}

std::string FileManager::obtenirCheminAbsolu(const std::string& cheminRelatif) {
    return obtenirCheminRacine() + "/" + cheminRelatif;
}

std::string FileManager::configJSONPath() {
    return obtenirCheminAbsolu("assets/settings/config.json");
}

std::string FileManager::favoritesJSONPath() {
    return obtenirCheminAbsolu("assets/datas/favorites.json");
}

std::string FileManager::historyJSONPath() {
    return obtenirCheminAbsolu("assets/datas/history.json");
}

std::string FileManager::webkitDataDirectory() {
    std::string dir = xdgDataHome() + "/weedlyweb/webkit-data";
    std::filesystem::create_directories(dir);
    return dir;
}

std::string FileManager::webkitCacheDirectory() {
    std::string dir = xdgCacheHome() + "/weedlyweb/webkit-cache";
    std::filesystem::create_directories(dir);
    return dir;
}

std::string FileManager::cookiesDatabasePath() {
    std::string dir = xdgDataHome() + "/weedlyweb";
    std::filesystem::create_directories(dir);
    return dir + "/cookies.sqlite";
}

std::string FileManager::cheminParametresHTML() {
    return obtenirCheminAbsolu("assets/settings/settings.html");
}

std::string FileManager::cheminHelpHTML() {
    return obtenirCheminAbsolu("assets/help/help.html");
}

std::string FileManager::cheminStylesCSS() {
    return obtenirCheminAbsolu("assets/styles/style.css");
}

nlohmann::json FileManager::readJSON(const std::string& chemin) {
    try {
        std::ifstream fichier(chemin);
        if (!fichier.is_open()) {
            std::cerr << "Erreur : Le fichier " << chemin << " est introuvable ou inaccessible." << std::endl;
            return nlohmann::json::object();
        }
        nlohmann::json contenu;
        fichier >> contenu;
        return contenu;
    } catch (const std::exception& e) {
        std::cerr << "Erreur lors de la lecture du fichier JSON (" << chemin << ") : " << e.what() << std::endl;
        return nlohmann::json::object();
    }
}

void FileManager::writeJSON(const std::string& chemin, const nlohmann::json& contenu) {
    try {
        std::ofstream fichier(chemin);
        if (!fichier.is_open()) {
            std::cerr << "Erreur : Impossible d'écrire dans le fichier " << chemin << std::endl;
            return;
        }
        fichier << contenu.dump(4);
    } catch (const std::exception& e) {
        std::cerr << "Erreur lors de l'écriture du fichier JSON (" << chemin << ") : " << e.what() << std::endl;
    }
}
