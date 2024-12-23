#include <filesystem>
#include <iostream>
#include "GestionnaireFichiers.h"

// Méthode pour obtenir la racine du projet
std::string GestionnaireFichiers::obtenirCheminRacine() {
    auto current_path = std::filesystem::current_path();
    std::cout << "Chemin racine détecté : " << current_path.string() << std::endl;
    return current_path.parent_path().string();
    //std::string cheminRacine = std::filesystem::path(__FILE__).parent_path().parent_path().string();
    //return cheminRacine + "/build";
}

// Retourne un chemin absolu en combinant le chemin actuel avec un chemin relatif
std::string GestionnaireFichiers::obtenirCheminAbsolu(const std::string& cheminRelatif) {
    return obtenirCheminRacine() + "/" + cheminRelatif;
}

// Chemin spécifique pour config.json
std::string GestionnaireFichiers::cheminConfigJSON() {
    return obtenirCheminAbsolu("assets/settings/config.json");
}

// Chemin spécifique pour favoris.json
std::string GestionnaireFichiers::cheminFavorisJSON() {
    return obtenirCheminAbsolu("assets/datas/favoris.json");
}

// Chemin spécifique pour parametres.html
std::string GestionnaireFichiers::cheminParametresHTML() {
    return obtenirCheminAbsolu("assets/settings/parametres.html");
}

// Chemin spécifique pour style.css
std::string GestionnaireFichiers::cheminStylesCSS() {
    return obtenirCheminAbsolu("assets/styles/style.css");
}

// Lecture d'un fichier JSON
nlohmann::json GestionnaireFichiers::lireJSON(const std::string& chemin) {
    try {
        std::ifstream fichier(chemin);
        if (!fichier.is_open()) {
            throw std::ios_base::failure("Impossible d'ouvrir le fichier en lecture : " + chemin);
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
void GestionnaireFichiers::ecrireJSON(const std::string& chemin, const nlohmann::json& contenu) {
    try {
        std::ofstream fichier(chemin);
        if (!fichier.is_open()) {
            throw std::ios_base::failure("Impossible d'ouvrir le fichier en écriture : " + chemin);
        }
        fichier << contenu.dump(4); // Ecrit avec un indentation de 4 espaces
    } catch (const std::exception& e) {
        std::cerr << "Erreur lors de l'écriture du fichier JSON (" << chemin << ") : " << e.what() << std::endl;
    }
}
