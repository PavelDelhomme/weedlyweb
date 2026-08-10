#ifndef FAVORITESMANAGER_H
#define FAVORITESMANAGER_H

#include <gtk/gtk.h>
#include <nlohmann/json.hpp>
#include <string>
#include <functional>
#include <memory>
#include <vector>

class FavoritesManager {
public:
    FavoritesManager(std::shared_ptr<nlohmann::json> favorites, std::function<void()> callbackRafraichir);
    ~FavoritesManager();

    void createInterface();
    void displayFavoritesList();
    void addFavorite(const std::string& nom, const std::string& url, const std::string& tag = "Général");
    void removeFavorite(const std::string& favoriteName);
    bool removeFavoriteFromList(const std::string& favoriteName);
    void modifyFavorite(const std::string& favoriteName, const std::string& newUrl);
    void modifyFavoriteFull(const std::string& oldName, const std::string& newName, const std::string& newUrl);
    void createFavoriteContextMenu(GtkWidget* button, const std::string& favoriteName);
    void saveModifications();
    void refreshInterface();
    void showWindow();
    void closeWindow();
    void addFolder(const std::string& nom);
    void addFavoriteToFolder(const std::string& folderName, const std::string& nom, const std::string& url, const std::string& tag = "");
    void moveItemToFolder(const std::string& itemName, const std::string& targetFolderName);

    void setPageContext(const std::string& title, const std::string& url);
    void openFolder(const std::string& folderName);
    void navigateBack();
    void navigateForward();
    void navigateUp();

    void showItemMenu(GtkWidget* anchor, const nlohmann::json& item);
    void promptRename(const std::string& name, const std::string& url, bool isFolder);
    void promptMove(const std::string& itemName);

    GtkWidget* getListeFavoris();
    GtkWidget* getFenetre() const { return window; }

private:
    std::shared_ptr<nlohmann::json> favorites;
    std::function<void()> callbackRafraichir;

    GtkWidget* window = nullptr;
    GtkWidget* favoritesList = nullptr;
    GtkWidget* scrolled = nullptr;
    GtkWidget* pathLabel = nullptr;
    GtkWidget* backButton = nullptr;
    GtkWidget* forwardButton = nullptr;
    GtkWidget* upButton = nullptr;
    GtkWidget* deleteButton = nullptr;
    GtkWidget* addForm = nullptr;
    GtkWidget* nameEntry = nullptr;
    GtkWidget* urlEntry = nullptr;
    GtkWidget* toolbar = nullptr;

    std::vector<std::string> currentPath;
    std::vector<std::vector<std::string>> backStack;
    std::vector<std::vector<std::string>> forwardStack;

    std::string pageTitle;
    std::string pageUrl;
    std::string selectedName;

    nlohmann::json& currentArray();
    void rebuildList();
    void updateNavButtons();
    void updatePathLabel();
    void updateDeleteVisibility();
    void showAddForm(bool show);
    void commitAddForm();
    void cancelAddForm();
    void pushHistoryBeforeNavigate();
    void selectRowByName(const std::string& name);
    GtkWidget* buildRow(const nlohmann::json& item);
};

#endif
