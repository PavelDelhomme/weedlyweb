#ifndef BROWSER_H
#define BROWSER_H

#include <memory>
#include <string>
#include <vector>
#include <set>
#include <nlohmann/json.hpp>
#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#include "rendering/RenderingEngine.h"
#include "managers/HTTPManager.h"
#include "managers/MemoryManager.h"
#include "engine/ScriptEngine.h"
#include "managers/FavoritesManager.h"
#include "managers/TabsManager.h"
#include "utils/CommandPalette.h"
#include "utils/RequestInterceptor.h"
#include "database/Database.h"

// Structure pour stocker les données d'un onglet
struct TabData {
    std::string url;
    GtkWidget* tabWidget;  // Le widget de l'onglet dans la barre
    WebKitWebView* webView;  // La WebView unique pour cet onglet
    GtkWidget* label;  // Le label avec le titre
    GtkWidget* faviconImage; // Favicon à gauche du titre
    std::string groupName;   // Groupe d'onglets (style Brave)
    
    TabData()
        : tabWidget(nullptr),
          webView(nullptr),
          label(nullptr),
          faviconImage(nullptr),
          groupName("Par défaut") {}
};

class Browser {
public:
    Browser();
    ~Browser();

    // Méthodes principales
    void launch();
    void saveConfiguration();
    void loadConfiguration();

    // Gestion de l'interface
    void buildInterface();
    void loadStyles();

    // Gestion des tabs
    TabsManager* getTabsManager() {
       return tabsManager.get();
    }
    void changeTabGroup(const std::string& groupName);
    void addNewTab(const std::string &url = "");
    void removeTab(GtkWidget* tabWidget);
    void changeActiveTab(GtkWidget* tabWidget);
    void moveTabToGroup(GtkWidget* tabWidget, const std::string& groupName);
    void refreshTabsBarVisibility();
    void focusUrlBar();
    void focusHomepageSearchBox(WebKitWebView* webView);
    void requestHomepageSearchFocus();
    bool consumeHomepageSearchFocus();
    void deleteTabGroup(const std::string& groupName);

    // Gestion des favorites
    void initializeFavoritesBar();
    void refreshFavoritesBar();
    void showFavoritesManager();
    void showFavoritesMenu();
    void showRemainingFavoritesMenu();
    void addFavorite(const std::string& nom, const std::string& url, const std::string& tag);
    void removeFavorite(GtkWidget* widget);
    void showCommandPalette();
    void toggleCommandPalette();  // Afficher/masquer la palette de commandes
    void showOptionsMenu();
    void showGroupsMenu();
    void showFavoritesOverflowMenu();
    void showSettings();
    void showHelp();
    void showHistoryPage();
    void showPasswordsPage();
    void showDevToolsPage(const std::string& tool);
    void openHtmlTab(const std::string& title, const std::string& html);
    void openAllBookmarksInFolder(const nlohmann::json& folder);
    bool confirmAction(const std::string& title, const std::string& message);
    void toggleCurrentPageFavorite();
    void persistSession();

    // Méthodes utilitaires
    void loadURL(const std::string& url);
    std::string getCurrentURL() const;
    std::string getCurrentTitle() const;
    
    // Méthodes internes liés à GTK
    static void onNavigateBackWrapper(GtkButton *button, gpointer user_data);
    static void onNavigateForwardWrapper(GtkButton *button, gpointer user_data);
    static void onRefreshPageWrapper(GtkButton *button, gpointer user_data);
    static void onUrlBarActivate(GtkEntry *entry, gpointer user_data);
    void onNavigateBack(GtkButton *button, Browser* navigateur);
    void onNavigateForward(GtkButton *button, Browser* navigateur);
    void onRefreshPage(GtkButton * button, Browser* navigateur);
    void onGoHome(GtkButton * button, Browser* navigateur);

    // Getters/Setters
    std::shared_ptr<nlohmann::json> getFavoris();
    std::string getHomepage() const { return homepage; }
    
    GtkWidget* getEntryNomFavori() { return favoriteNameEntry; }
    GtkWidget* getEntryURLFavori() { return favoriteUrlEntry; }
    GtkWidget* getPopoverFavoris() { return favoritesPopover; }
    
    // Méthode publique pour gérer l'état de chargement
    void setLoadingState(bool loading);

    // Interface utilisateur
    void closeApplication();


private:
    // Membres d'interface utilisateur
    GtkWidget *window;
    GtkWidget *mainContainer;
    GtkWidget *navigationBar;
    GtkWidget *favoritesBar;
    GtkWidget *tabsBar;
    GtkWidget *groupChip;      // Pastille colorée du groupe actif
    GtkWidget *groupChipLabel; // Nom du groupe actif
    GtkWidget *urlBar;
    GtkWidget *starButton;
    GtkWidget *loadingSpinner;  // Indicateur de chargement dans la barre d'URL
    GtkWidget *favoriteNameEntry;
    GtkWidget *favoriteUrlEntry;
    GtkWidget *favoritesPopover;

    // Composants internes
    std::unique_ptr<RenderingEngine> renderingEngine;
    std::unique_ptr<HTTPManager> httpManager;
    std::unique_ptr<MemoryManager> memoryManager;
    std::unique_ptr<ScriptEngine> scriptEngine;
    std::unique_ptr<FavoritesManager> favoritesManager;
    std::unique_ptr<TabsManager> tabsManager;
    std::unique_ptr<CommandPalette> commandPalette;
    std::unique_ptr<RequestInterceptor> requestInterceptor;
    std::unique_ptr<Database> database;

    // Données
    std::vector<std::string> history;
    std::vector<TabData> tabs;  // Chaque onglet a sa propre WebView
    TabData* activeTab;  // Pointeur vers l'onglet actif
    GtkWidget* webContainer;  // Container pour les WebViews (une seule visible à la fois)
    std::shared_ptr<nlohmann::json> favorites;
    std::string homepage;
    bool pendingHomepageSearchFocus = false;
    std::vector<std::string> pendingSessionTabs;
    bool restorePreviousSession = false;

    // Méthodes internes
    void initializeNavigationBar();
    void initializeTabsBar();
    void updateGroupChip();
    void showTabContextMenu(GtkWidget* tabWidget, GdkEventButton* event);
    void initializeFavoritesPopover();
    void highlight(GtkWidget* tabWidget);
    void addButton(GtkWidget* container, const std::string& iconName, GCallback callback, gpointer data);
    void updateStarButton();
    void restoreSessionTabs();
    void executeScriptInActiveTab(const std::string& script);
    void showMessage(const std::string& message);
    void createContextMenu(GtkWidget* button);
    static void onStarButtonClicked(GtkButton* button, gpointer user_data);
    static std::string normalizeNavigationUrl(const std::string& url);

    // Gestionnaire de mémoire et signaux
    void configureKeyboardShortcuts(); 
};

#endif
