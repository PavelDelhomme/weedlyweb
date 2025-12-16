#include <filesystem>
#include <iostream>
#include "managers/FileManager.h"

// Méthode pour obtenir la racine du projet
std::string FileManager::obtenirCheminRacine() {
    auto current_path = std::filesystem::current_path();
    
    // Si exécuté depuis le dossier build
    if (current_path.filename() == "build") {
        return current_path.parent_path().string(); // Retourne le chemin racine
    }

    // Sinon, retourne le chemin actuel
    return current_path.string();
}


// Retourne un chemin absolu en combinant le chemin actuel avec un chemin relatif
std::string FileManager::obtenirCheminAbsolu(const std::string& cheminRelatif) {
    return obtenirCheminRacine() + "/" + cheminRelatif;
}

// Chemin spécifique pour config.json
std::string FileManager::configJSONPath() {
    return obtenirCheminAbsolu("assets/settings/config.json");
}

// Chemin spécifique pour favorites.json
std::string FileManager::favoritesJSONPath() {
    return obtenirCheminAbsolu("assets/datas/favorites.json");
}

// Chemin spécifique pour settings.html
std::string FileManager::cheminParametresHTML() {
    return obtenirCheminAbsolu("assets/settings/settings.html");
}

// Chemin spécifique pour help.html
std::string FileManager::cheminHelpHTML() {
    return obtenirCheminAbsolu("assets/help/help.html");
}

// Chemin spécifique pour style.css
std::string FileManager::cheminStylesCSS() {
    return obtenirCheminAbsolu("assets/styles/style.css");
}

// Lecture d'un fichier JSON
nlohmann::json FileManager::readJSON(const std::string& chemin) {
    try {
        std::ifstream fichier(chemin);
        if (!fichier.is_open()) {
            std::cerr << "Erreur : Le fichier " << chemin << " est introuvable ou inaccessible." << std::endl;
            return nlohmann::json::object(); // Retourne un objet JSON vide
        }
        nlohmann::json contenu;
        fichier >> contenu;
        return contenu;
    } catch (const std::exception& e) {
        std::cerr << "Erreur lors de la lecture du fichier JSON (" << chemin << ") : " << e.what() << std::endl;
        return nlohmann::json::object();
    }
}

// Écriture d'un fichier JSON
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
