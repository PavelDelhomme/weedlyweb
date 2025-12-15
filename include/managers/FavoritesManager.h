#ifndef FAVORITESMANAGER_H
#define FAVORITESMANAGER_H

#include <gtk/gtk.h>
#include <nlohmann/json.hpp>
#include <string>
#include <functional>

class FavoritesManager {
public:
    // FavoritesManager(nlohmann::json& favorites, std::function<void()> callbackRafraichir);
    FavoritesManager(std::shared_ptr<nlohmann::json> favorites, std::function<void()> callbackRafraichir);
    ~FavoritesManager();
    void createInterface();
    void displayFavoritesList();
    void addFavorite(const std::string& nom, const std::string& url, const std::string& tag);
    void removeFavorite(const std::string& favoriteName);
    bool removeFavoriteFromList(const std::string& favoriteName);
    void modifyFavorite(const std::string& favoriteName, const std::string& newUrl);
    void createFavoriteContextMenu(GtkWidget* button, const std::string& favoriteName);
    void saveModifications();
    void refreshInterface();
    void showWindow();
    void closeWindow();
    void addFolder(const std::string& nom);
    // void ajouterFavoriDansDossier(const std::string& dossier, const std::string& nom, const std::string& url);
    GtkWidget* getListeFavoris();  // Déclaration d'un accesseur
    GtkWidget* getFenetre() const { return window; }
    
private:
    // nlohmann::json& favorites;
    std::shared_ptr<nlohmann::json> favorites;
    std::function<void()> callbackRafraichir;
    GtkWidget* window;
    GtkWidget* favoritesList;
    // GtkWidget *window = nullptr;
    // GtkWidget *favoritesList = nullptr;
    // GtkWidget *formulaireModification = nullptr;
    // nlohmann::json& favorites;
    // std::function<void()> callbackRafraichir;
    // static void on_bouton_fermer_clicked(GtkWidget*, gpointer user_data);
};

#endif
