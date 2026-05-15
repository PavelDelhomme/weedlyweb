#include "browser/Browser.h"
#include "managers/FileManager.h"
#include "managers/MemoryManager.h"
#include "managers/FavoritesManager.h"
#include "utils/Utils.h"
#include "utils/CommandPalette.h"
#include "utils/RequestInterceptor.h"
#include "database/Database.h"
#include <iostream>
#include <filesystem>
#include <string>
#include <fstream>
#include <set>
#include <cstdlib>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdk.h>

namespace {
bool isDebugLoggingEnabled() {
    const char* value = std::getenv("WEEDLYWEB_DEBUG");
    if (!value) {
        return false;
    }

    std::string normalized(value);
    return normalized != "0" && normalized != "false" && normalized != "FALSE" &&
           normalized != "off" && normalized != "OFF";
}
}

#define WEEDLYWEB_DEBUG_LOG(message) \
    do { \
        if (isDebugLoggingEnabled()) { \
            std::cerr << message << std::endl; \
        } \
    } while (false)

// Variable globale pour le callback de chargement
Browser* g_browser_instance = nullptr;

// Fonction helper pour gérer l'état de chargement dans la barre d'URL
void browser_set_loading_state(bool loading) {
    if (g_browser_instance) {
        g_browser_instance->setLoadingState(loading);
    }
}

// Méthode publique pour gérer l'état de chargement
void Browser::setLoadingState(bool loading) {
    if (loadingSpinner) {
        if (loading) {
            gtk_widget_show(loadingSpinner);
            gtk_spinner_start(GTK_SPINNER(loadingSpinner));
        } else {
            gtk_spinner_stop(GTK_SPINNER(loadingSpinner));
            gtk_widget_hide(loadingSpinner);
        }
    }
}

// Fonction utilitaire pour obtenir le chemin absolu d'un fichier
// Conforme aux standards C++17 : const-correctness et référence const
std::string obtenirCheminAbsolu(const std::string& fichier) {
    return std::filesystem::current_path().string() + "/" + fichier;
}
static void delete_user_data(gpointer user_data, GClosure*) {
    delete static_cast<std::pair<Browser*, GtkWidget*>*>(user_data);
}

// Déclarations pour Utils.cpp (non-static pour être accessibles depuis Utils.cpp)
void on_ouvrir_nouvel_onglet_safe(GtkWidget*, gpointer user_data);
void on_modifier_favori(GtkWidget*, gpointer user_data);
void on_supprimer_favori(GtkWidget*, gpointer user_data);

static gboolean on_favori_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    if (event->type == GDK_BUTTON_PRESS && event->button == GDK_BUTTON_SECONDARY) {
        auto* navigateur = static_cast<Browser*>(user_data);
        GtkWidget* menu = creerMenuContextuelFavoris(navigateur, widget);
        gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent*)event);
        return TRUE;
    }
    return FALSE;
}



// Définition pour Utils.cpp
void on_ouvrir_nouvel_onglet_safe(GtkWidget*, gpointer user_data) {
    auto* data = static_cast<std::pair<Browser*, std::string>*>(user_data);
    if (!data) return;
    if (data->first) {
        data->first->addNewTab(data->second);
    }
    delete data;
}

// Définition pour Utils.cpp
void on_supprimer_favori(GtkWidget* widget, gpointer user_data) {
    auto* data = static_cast<std::pair<Browser*, GtkWidget*>*>(user_data);
    if (data && data->first) {
        data->first->removeFavorite(data->second);
    }
    delete data;
}


static void on_menu_item_activate(GtkWidget* widget, gpointer user_data) {
    auto* data = static_cast<std::pair<Browser*, std::string>*>(user_data);
    data->first->loadURL(data->second);
    delete data;
}

static void free_browser_url_pair(gpointer data, GClosure*) {
    delete static_cast<std::pair<Browser*, std::string>*>(data);
}

static std::string truncate_favorite_label(const std::string& s) {
    constexpr std::size_t kMax = 12;
    if (s.size() <= kMax) {
        return s;
    }
    return s.substr(0, kMax - 3) + "...";
}

static void populate_favorites_menu_from_children(GtkMenuShell* shell, Browser* browser, const nlohmann::json& children);

static void populate_favorites_menu_from_children(GtkMenuShell* shell, Browser* browser, const nlohmann::json& children) {
    if (!children.is_array() || children.empty()) {
        GtkWidget* emptyItem = gtk_menu_item_new_with_label("(vide)");
        gtk_widget_set_sensitive(emptyItem, FALSE);
        gtk_menu_shell_append(shell, emptyItem);
        return;
    }
    for (const auto& child : children) {
        if (FavoritesJson::isFolder(child)) {
            const std::string folderName = child.value("name", "Dossier");
            GtkWidget* mi = gtk_menu_item_new_with_label(folderName.c_str());
            GtkWidget* sub = gtk_menu_new();
            nlohmann::json subChildren = nlohmann::json::array();
            if (child.contains("children") && child["children"].is_array()) {
                subChildren = child["children"];
            }
            populate_favorites_menu_from_children(GTK_MENU_SHELL(sub), browser, subChildren);
            gtk_menu_item_set_submenu(GTK_MENU_ITEM(mi), sub);
            gtk_menu_shell_append(shell, mi);
        } else if (child.contains("url") && child.contains("name") && child["url"].is_string()) {
            const std::string n = child["name"].get<std::string>();
            const std::string u = child["url"].get<std::string>();
            GtkWidget* mi = gtk_menu_item_new_with_label(n.c_str());
            auto* data = new std::pair<Browser*, std::string>(browser, u);
            g_signal_connect(mi, "activate", G_CALLBACK(on_menu_item_activate), data);
            gtk_menu_shell_append(shell, mi);
        }
    }
}

static GtkWidget* create_folder_menu_button(Browser* browser, const nlohmann::json& folderItem) {
    GtkWidget* mb = gtk_menu_button_new();
    const std::string baseName = folderItem.value("name", "Dossier");
    const std::string label = truncate_favorite_label(baseName);
    gtk_menu_button_set_direction(GTK_MENU_BUTTON(mb), GTK_ARROW_DOWN);
    gtk_button_set_label(GTK_BUTTON(mb), label.c_str());
    gtk_widget_set_tooltip_text(mb, baseName.c_str());
    gtk_widget_set_name(mb, "button-favori-dossier");
    GtkWidget* menu = gtk_menu_new();
    nlohmann::json children = nlohmann::json::array();
    if (folderItem.contains("children") && folderItem["children"].is_array()) {
        children = folderItem["children"];
    }
    populate_favorites_menu_from_children(GTK_MENU_SHELL(menu), browser, children);
    gtk_menu_button_set_popup(GTK_MENU_BUTTON(mb), menu);
    gtk_widget_set_margin_start(mb, 1);
    gtk_widget_set_margin_end(mb, 1);
    return mb;
}

static void on_bookmark_load_url(GtkButton*, gpointer user_data) {
    auto* p = static_cast<std::pair<Browser*, std::string>*>(user_data);
    if (p && p->first) {
        p->first->loadURL(p->second);
    }
}

static void connect_bookmark_button_clicked(GtkWidget* boutonFavori, Browser* browser, const std::string& url) {
    auto* data = new std::pair<Browser*, std::string>(browser, url);
    g_signal_connect_data(
        boutonFavori,
        "clicked",
        G_CALLBACK(on_bookmark_load_url),
        data,
        free_browser_url_pair,
        (GConnectFlags)0);
}


static void on_favori_destroy(GtkWidget* widget, gpointer user_data) {
    g_free(user_data);
}

static void on_destroy_callback(GtkWidget* widget, gpointer user_data) {
    delete static_cast<std::pair<Browser*, std::string>*>(user_data);
}


static void on_ajouter_onglet(GtkButton* button, gpointer user_data) {
    auto* navigateur = static_cast<Browser*>(user_data);
    if (navigateur) {
        navigateur->addNewTab(navigateur->getHomepage());
    }
}

static void on_page_chargee(GtkLabel* label, const std::string& titre) {
    gtk_label_set_text(label, titre.c_str());
}


static void on_naviguer_retour(GtkButton* button, gpointer user_data) {
    auto* navigateur = static_cast<Browser*>(user_data);
    navigateur->onNavigateBack(button, navigateur);
}

static void on_naviguer_suivant(GtkButton* button, gpointer user_data) {
    auto* navigateur = static_cast<Browser*>(user_data);
    navigateur->onNavigateForward(button, navigateur);
}

static void on_aller_accueil(GtkButton* button, gpointer user_data) {
    auto* navigateur = static_cast<Browser*>(user_data);
    navigateur->onGoHome(button, navigateur);
}

static void on_rafraichir_page(GtkButton* button, gpointer user_data) {
    auto* navigateur = static_cast<Browser*>(user_data);
    navigateur->onRefreshPage(button, navigateur);
}
// Fonction statique pour gérer l'ouverture dans un nouvel onglet
static void on_ouvrir_nouvel_onglet(GtkWidget*, gpointer user_data) {
    auto* data = static_cast<std::pair<Browser*, std::string>*>(user_data);
    data->first->addNewTab(data->second);
    delete data;
}
// Définition pour Utils.cpp
void on_modifier_favori(GtkWidget*, gpointer user_data) {
    auto* navigateur = static_cast<Browser*>(user_data);
    navigateur->showFavoritesManager();
}


static void on_ajouter_favori_menu(GtkWidget*, gpointer user_data) {
    auto* data = static_cast<std::pair<Browser*, std::pair<GtkWidget*, GtkWidget*>>*>(user_data);
    auto* navigateur = data->first;
    GtkWidget* entryNom = data->second.first;
    GtkWidget* entryURL = data->second.second;

    const gchar* nom = gtk_entry_get_text(GTK_ENTRY(entryNom));
    const gchar* url = gtk_entry_get_text(GTK_ENTRY(entryURL));

    navigateur->addFavorite(nom, url, "Général");
    delete data;
}


static void on_bouton_favoris_clicked(GtkButton*, gpointer user_data) {
    auto* navigateur = static_cast<Browser*>(user_data);
    if (navigateur) {
        std::string url = navigateur->getCurrentURL();
        if (!url.empty()) {
            navigateur->addFavorite("Favori", url, "");
            navigateur->refreshFavoritesBar();
        }
    }
}

static void on_ajouter_groupe(GtkWidget* widget, gpointer data) {
    auto* info = static_cast<std::pair<Browser*, GtkWidget*>*>(data);
    if (info && info->first) {
        const gchar* groupName = gtk_entry_get_text(GTK_ENTRY(info->second));
        info->first->getTabsManager()->ajouterGroupe(groupName);
        info->first->changeTabGroup(groupName);
        delete info;
    }
}

static void on_changer_groupe(GtkWidget* item, gpointer data) {
    auto* navigateur = static_cast<Browser*>(data);
    if (navigateur) {
        const char* groupName = gtk_menu_item_get_label(GTK_MENU_ITEM(item));
        navigateur->changeTabGroup(groupName);
    }
}

static gboolean on_delete_event(GtkWidget*, GdkEvent*, gpointer user_data) {
    auto* navigateur = static_cast<Browser*>(user_data);
    navigateur->saveConfiguration();
    gtk_main_quit();
    return FALSE;
}

static gboolean on_key_press(GtkWidget*, GdkEvent* event, gpointer user_data) {
    auto* navigateur = static_cast<Browser*>(user_data);
    
    if (event->type == GDK_KEY_PRESS) {
        GdkEventKey* key_event = (GdkEventKey*) event;

        // Gestion du raccourci CTRL + T pour ouvrir un nouvel onglet
        if ((key_event->state & GDK_CONTROL_MASK) && key_event->keyval == GDK_KEY_t) {
            navigateur->addNewTab(navigateur->getHomepage());
            return TRUE;
        }

        // Gestion du raccourci CTRL + D pour ajouter un favori
        if ((key_event->state & GDK_CONTROL_MASK) && key_event->keyval == GDK_KEY_d) {
            std::string url = navigateur->getCurrentURL();
            std::string titre = navigateur->getCurrentTitle();
            
            // Pré-remplir les champs du formulaire
            gtk_entry_set_text(GTK_ENTRY(navigateur->getEntryNomFavori()), titre.c_str());
            gtk_entry_set_text(GTK_ENTRY(navigateur->getEntryURLFavori()), url.c_str());

            // Afficher le formulaire d'ajout de favori (popover)
            gtk_popover_popup(GTK_POPOVER(navigateur->getPopoverFavoris()));
            return TRUE;
        }
        
        // Gestion du raccourci CTRL + SHIFT + C pour afficher/masquer la palette de commandes
        if ((key_event->state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) && 
            key_event->keyval == GDK_KEY_c) {
            navigateur->toggleCommandPalette();
            return TRUE;
        }
        
        // Gestion du raccourci CTRL + H pour afficher l'aide
        if ((key_event->state & GDK_CONTROL_MASK) && 
            key_event->keyval == GDK_KEY_h) {
            navigateur->showHelp();
            return TRUE;
        }
        
        // Gestion du raccourci CTRL + SHIFT + D pour dupliquer l'onglet actuel
        if ((key_event->state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) && 
            key_event->keyval == GDK_KEY_d) {
            std::string url = navigateur->getCurrentURL();
            if (!url.empty()) {
                navigateur->addNewTab(url);
            }
            return TRUE;
        }
    }
    return FALSE;
}


// ✅ Gestion du clic droit (menu contextuel)
static gboolean on_favoris_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    if (event->type == GDK_BUTTON_PRESS && event->button == GDK_BUTTON_SECONDARY) {  // Clic droit détecté
        auto* navigateur = static_cast<Browser*>(user_data);
        if (!navigateur) return FALSE;

        // Créer un menu contextuel
        GtkWidget *menu = gtk_menu_new();
        
        // **Option 1 : Ouvrir dans un nouvel onglet**
        auto data = std::make_unique<std::pair<Browser*, std::string>>(navigateur, navigateur->getCurrentURL());
        GtkWidget *ouvrirNouvelOnglet = gtk_menu_item_new_with_label("Ouvrir dans un nouvel onglet");
        g_signal_connect_data(ouvrirNouvelOnglet, "activate", G_CALLBACK(on_ouvrir_nouvel_onglet_safe), data.release(), delete_user_data, G_CONNECT_AFTER);

        gtk_menu_shell_append(GTK_MENU_SHELL(menu), ouvrirNouvelOnglet);

        // **Option 2 : Modifier le favori**
        GtkWidget *modifierItem = gtk_menu_item_new_with_label("Modifier le favori");
        g_signal_connect(modifierItem, "activate", G_CALLBACK(on_modifier_favori), navigateur);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), modifierItem);

        // **Option 3 : Supprimer le favori**
        GtkWidget *supprimerItem = gtk_menu_item_new_with_label("Supprimer le favori");
        g_signal_connect_data(supprimerItem, "activate", G_CALLBACK(on_supprimer_favori),
                         new std::pair<Browser*, GtkWidget*>(navigateur, widget),
                        delete_user_data,
                         G_CONNECT_AFTER);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), supprimerItem);

        // **Afficher le menu contextuel au clic droit**
        gtk_widget_show_all(menu);
        gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent*)event); 
        return TRUE;  // Événement capturé
    }
    return FALSE;  // Événement non capturé
}

void on_button_ajouter_clicked(GtkButton* button, gpointer user_data) {
    auto* navigateur = static_cast<Browser*>(user_data);

    // Récupérer les valeurs des champs
    const gchar* nom = gtk_entry_get_text(GTK_ENTRY(navigateur->getEntryNomFavori()));
    const gchar* url = gtk_entry_get_text(GTK_ENTRY(navigateur->getEntryURLFavori()));

    if (nom && url && *nom && *url) {
        // Ajout du favori dans la liste
        navigateur->addFavorite(nom, url, "Général");
        navigateur->refreshFavoritesBar();

        // Cacher le popover via la méthode publique
        gtk_widget_hide(navigateur->getPopoverFavoris());
    } else {
        std::cerr << "Veuillez remplir les deux champs." << std::endl;
    }
}




Browser::Browser() 
    : renderingEngine(std::make_unique<RenderingEngine>()),
      httpManager(std::make_unique<HTTPManager>()),
      memoryManager(std::make_unique<MemoryManager>()),
      tabsManager(std::make_unique<TabsManager>()),
      scriptEngine(std::make_unique<ScriptEngine>()),
      favorites(std::make_shared<nlohmann::json>()),
      window(nullptr),
      mainContainer(nullptr),
      navigationBar(nullptr),
      favoritesBar(nullptr),
      tabsBar(nullptr),
      urlBar(nullptr),
      starButton(nullptr),
      loadingSpinner(nullptr),
      favoriteNameEntry(nullptr),
      favoriteUrlEntry(nullptr),
      favoritesPopover(nullptr),
      activeTab(nullptr),
      webContainer(nullptr)
{
    // Initialiser la base de données
    database = std::make_unique<Database>();
    if (!database->initDatabase()) {
        std::cerr << "Erreur lors de l'initialisation de la base de données" << std::endl;
    }
    
    // Initialiser l'intercepteur de requêtes
    requestInterceptor = std::make_unique<RequestInterceptor>();
    // Temporairement désactivé pour déboguer le crash
    // requestInterceptor->enable();
    
    // Initialiser la palette de commandes
    commandPalette = std::make_unique<CommandPalette>();
    commandPalette->setRequestInterceptor(requestInterceptor.get());
    
    favoritesManager = std::make_unique<FavoritesManager>(favorites, [this]() { refreshFavoritesBar(); });
    loadConfiguration();
    buildInterface();
    g_signal_connect(window, "key-press-event", G_CALLBACK(on_key_press), this);
}


Browser::~Browser() {
    // Sauvegarder la configuration avant de fermer
    saveConfiguration();
    
    // Nettoyer les WebViews et leurs signaux avant de détruire les widgets
    // Les WebViews sont gérées par GTK mais on doit nettoyer les références
    for (auto& tab : tabs) {
        if (tab.webView && WEBKIT_IS_WEB_VIEW(tab.webView)) {
            // Déconnecter les signaux de la WebView
            g_signal_handlers_disconnect_matched(tab.webView, G_SIGNAL_MATCH_DATA, 0, 0, nullptr, nullptr, this);
            // Note: Ne pas appeler g_object_unref car GTK gère la durée de vie des widgets
        }
        tab.webView = nullptr;
        tab.tabWidget = nullptr;
        tab.label = nullptr;
    }
    tabs.clear();
    activeTab = nullptr;
    
    // Nettoyer le moteur de rendu en premier (qui nettoie ses propres signaux)
    if (renderingEngine) {
        renderingEngine.reset();
    }
    
    // Nettoyer les signaux avant de détruire les widgets
    if (window && GTK_IS_WIDGET(window)) {
        // Vérifier que le widget n'est pas déjà en cours de destruction
        if (!gtk_widget_in_destruction(window)) {
            // Déconnecter tous les signaux de la fenêtre
            g_signal_handlers_disconnect_matched(window, G_SIGNAL_MATCH_DATA, 0, 0, nullptr, nullptr, this);
            
            // Détruire la fenêtre (cela détruira automatiquement tous les children)
            gtk_widget_destroy(window);
        }
        window = nullptr;
    }
    
    // Réinitialiser les pointeurs pour éviter les accès après destruction
    mainContainer = nullptr;
    navigationBar = nullptr;
    favoritesBar = nullptr;
    tabsBar = nullptr;
    urlBar = nullptr;
    starButton = nullptr;
    loadingSpinner = nullptr;
    favoriteNameEntry = nullptr;
    favoriteUrlEntry = nullptr;
    favoritesPopover = nullptr;
    webContainer = nullptr;
}

std::shared_ptr<nlohmann::json> Browser::getFavoris() {
    return favorites;
}

void Browser::buildInterface() {
    // Debug messages removed for cleaner output
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "WeedlyWeb");
    
    // Forcer le mode sombre sur la fenêtre (utilise le thème système)
    GtkSettings* settings = gtk_settings_get_default();
    g_object_set(settings, "gtk-application-prefer-dark-theme", TRUE, NULL);
    
    // Activer le redimensionnement de la fenêtre
    gtk_window_set_resizable(GTK_WINDOW(window), TRUE);
    
    // Obtenir la taille de l'écran immédiatement pour définir la taille par défaut
    GdkDisplay* display = gdk_display_get_default();
    if (display) {
        GdkMonitor* monitor = gdk_display_get_primary_monitor(display);
        if (!monitor) {
            gint n_monitors = gdk_display_get_n_monitors(display);
            if (n_monitors > 0) {
                monitor = gdk_display_get_monitor(display, 0);
            }
        }
        
        if (monitor) {
            GdkRectangle geometry;
            gdk_monitor_get_geometry(monitor, &geometry);
            // Utiliser 90% de la largeur et 90% de la hauteur pour mieux utiliser l'écran
            gint windowWidth = (geometry.width * 90) / 100;
            gint windowHeight = (geometry.height * 90) / 100;
            gtk_window_set_default_size(GTK_WINDOW(window), windowWidth, windowHeight);
            
            // Centrer la fenêtre
            gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
        } else {
            // Fallback si pas de moniteur - utiliser une taille raisonnable
            gtk_window_set_default_size(GTK_WINDOW(window), 1920, 1080);
        }
    } else {
        // Fallback si pas de display
        gtk_window_set_default_size(GTK_WINDOW(window), 1920, 1080);
    }
    
    // Ajouter le support du plein écran avec F11
    g_signal_connect(window, "key-press-event", G_CALLBACK(+[](GtkWidget* widget, GdkEvent* event, gpointer user_data) -> gboolean {
        if (event->type == GDK_KEY_PRESS) {
            GdkEventKey* key_event = (GdkEventKey*) event;
            if (key_event->keyval == GDK_KEY_F11) {
                GtkWindow* window = GTK_WINDOW(widget);
                if (gtk_window_is_maximized(window)) {
                    gtk_window_unmaximize(window);
                } else {
                    gtk_window_maximize(window);
                }
                return TRUE;
            }
        }
        return FALSE;
    }), this);
    
    // Définir l'icône de la fenêtre
    std::string iconPath = FileManager::obtenirCheminAbsolu("assets/icons/weedlyweb.png");
    if (std::filesystem::exists(iconPath)) {
        GdkPixbuf *icon = gdk_pixbuf_new_from_file(iconPath.c_str(), nullptr);
        if (icon) {
            gtk_window_set_icon(GTK_WINDOW(window), icon);
            g_object_unref(icon);
        }
    } else {
        // Essayer avec le SVG si PNG n'existe pas
        std::string svgPath = FileManager::obtenirCheminAbsolu("assets/icons/weedlyweb.svg");
        if (std::filesystem::exists(svgPath)) {
            GdkPixbuf *icon = gdk_pixbuf_new_from_file(svgPath.c_str(), nullptr);
            if (icon) {
                gtk_window_set_icon(GTK_WINDOW(window), icon);
                g_object_unref(icon);
            }
        }
    }
    
    // Configuration pour que la fenêtre apparaisse dans la barre des tâches
    // Définir le nom de classe X11 pour l'identification par le gestionnaire de fenêtres
    gtk_widget_set_name(window, "weedlyweb");
    
    // Définir le rôle de la fenêtre (pour le gestionnaire de fenêtres)
    gtk_window_set_role(GTK_WINDOW(window), "weedlyweb-browser");
    
    // S'assurer que la fenêtre n'est pas ignorée par le gestionnaire de fenêtres
    // (par défaut, GTK_WINDOW_TOPLEVEL devrait déjà être visible, mais on s'en assure)
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(window), FALSE);
    gtk_window_set_skip_pager_hint(GTK_WINDOW(window), FALSE);
    
    // Définir le type de fenêtre (normal, pas un splash ou un popup)
    gtk_window_set_type_hint(GTK_WINDOW(window), GDK_WINDOW_TYPE_HINT_NORMAL);
    

    // Connexion sécurisée du signal de fermeture avec lambda sécurisée
    g_signal_connect(window, "delete-event", G_CALLBACK(+[](GtkWidget*, GdkEvent*, gpointer user_data) -> gboolean {
        auto* navigateur = static_cast<Browser*>(user_data);
        navigateur->saveConfiguration();
        gtk_main_quit();  // Quitter proprement l'application
        return FALSE;  
    }), this);

    // Conteneur principal
    mainContainer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window), mainContainer);

    // CORRECT ORDER: Bars at top, web view at bottom (expandable)
    // 1. Tabs bar (at top)
    initializeTabsBar();
    
    // 2. Navigation bar with URL (under tabs)
    initializeNavigationBar();
    
    // 3. Favorites bar (under URL bar)
    refreshFavoritesBar();
    
    // 4. Web container (at bottom, expandable) - MUST be last to take remaining space
    // NOUVELLE APPROCHE : Container simple et direct
    webContainer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_name(webContainer, "container-web");
    gtk_widget_set_vexpand(webContainer, TRUE);
    gtk_widget_set_hexpand(webContainer, TRUE);
    
    // S'assurer que le container est visible dès le début
    gtk_widget_show_all(webContainer);
    
    if (mainContainer && !gtk_widget_get_parent(webContainer)) {
        // Utiliser pack_end pour que le container web prenne tout l'espace restant
        gtk_box_pack_end(GTK_BOX(mainContainer), webContainer, TRUE, TRUE, 0);
    }
    
    // Initialiser le moteur de rendu (passe le webContainer)
    renderingEngine->initializeRendering(webContainer);
    
    // Charger les styles CSS pour un design minimaliste
    loadStyles();
    
    // Initialiser l'instance globale pour le callback de chargement
    g_browser_instance = this;
    
    // Afficher la fenêtre
    gtk_widget_show_all(window);
    
    // Présenter la fenêtre au gestionnaire de fenêtres (pour qu'elle apparaisse dans la barre des tâches)
    gtk_window_present(GTK_WINDOW(window));
    
    // Forcer le traitement des événements GTK pour s'assurer que la fenêtre est rendue
    while (gtk_events_pending()) {
        gtk_main_iteration();
    }
    
    // Ajouter le premier onglet et charger la page d'accueil (APRÈS que la fenêtre soit visible)
    std::string homepageUrl = homepage.empty() ? "https://www.duckduckgo.com" : homepage;
    addNewTab(homepageUrl);

    renderingEngine->connectURLChangedSignal([this](const std::string& url) {
        if (urlBar) {
            gtk_entry_set_text(GTK_ENTRY(urlBar), url.c_str());
        }
        // Mettre à jour le bouton étoile selon si l'URL est en favoris
        updateStarButton();
    });

    initializeFavoritesPopover();
    
    // Mettre à jour le bouton étoile initial
    updateStarButton();
    // gtk_widget_show_all sera appelé après l'ajout de l'onglet
}

void Browser::addButton(GtkWidget* container, const std::string& iconName, GCallback callback, gpointer data) {
    GtkWidget *button = renderingEngine->createButton(iconName, callback, data);
    if (!gtk_widget_get_parent(button)) {  // Éviter les duplications
        gtk_box_pack_start(GTK_BOX(container), button, FALSE, FALSE, 0);
    }
}

std::string Browser::getCurrentTitle() const {
    if (activeTab && activeTab->webView) {
        const gchar* title = webkit_web_view_get_title(activeTab->webView);
        return title ? std::string(title) : "Titre inconnu";
    }
    return "Titre inconnu";
}



GtkWidget* obtenirDernierEnfant(GtkWidget* parent) {
    GList* children = gtk_container_get_children(GTK_CONTAINER(parent));
    return g_list_last(children) ? GTK_WIDGET(g_list_last(children)->data) : nullptr;
}

GtkWidget* obtenirPremierEnfant(GtkWidget* parent) {
    GList* children = gtk_container_get_children(GTK_CONTAINER(parent));
    return children ? GTK_WIDGET(children->data) : nullptr;
}

void Browser::initializeNavigationBar() {
    navigationBar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_widget_set_margin_start(navigationBar, 5);
    gtk_widget_set_margin_end(navigationBar, 5);
    gtk_widget_set_margin_top(navigationBar, 5);
    gtk_widget_set_margin_bottom(navigationBar, 5);
    gtk_widget_set_name(navigationBar, "barre-navigation");

    // Boutons de navigation (retour, suivant, rafraîchir, accueil)
    addButton(navigationBar, "go-previous", G_CALLBACK(on_naviguer_retour), this);
    addButton(navigationBar, "go-next", G_CALLBACK(on_naviguer_suivant), this);
    addButton(navigationBar, "view-refresh", G_CALLBACK(on_rafraichir_page), this);
    addButton(navigationBar, "go-home", G_CALLBACK(on_aller_accueil), this);
    
    // Container pour la barre d'URL avec indicateur de chargement
    GtkWidget* urlContainer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3);
    gtk_widget_set_name(urlContainer, "url-container");
    
    // Indicateur de chargement (spinner) dans la barre d'URL
    loadingSpinner = gtk_spinner_new();
    gtk_widget_set_size_request(loadingSpinner, 16, 16);
    gtk_widget_set_margin_start(loadingSpinner, 5);
    gtk_widget_set_margin_end(loadingSpinner, 5);
    gtk_widget_hide(loadingSpinner); // Masqué par défaut
    gtk_box_pack_start(GTK_BOX(urlContainer), loadingSpinner, FALSE, FALSE, 0);
    
    // Barre d'URL (expandable)
    urlBar = renderingEngine->createTextEntry(G_CALLBACK(&Browser::onUrlBarActivate), this);
    gtk_box_pack_start(GTK_BOX(urlContainer), urlBar, TRUE, TRUE, 0);
    
    // Ajouter le container URL à la barre de navigation
    gtk_box_pack_start(GTK_BOX(navigationBar), urlContainer, TRUE, TRUE, 0);

    // Bouton favorites (étoile) - visible et mis à jour selon l'état
    starButton = gtk_button_new_with_label("☆");
    gtk_widget_set_tooltip_text(starButton, "Ajouter aux favoris");
    gtk_widget_set_margin_start(starButton, 3);
    gtk_widget_set_margin_end(starButton, 3);
    g_signal_connect(starButton, "clicked", G_CALLBACK(on_bouton_favoris_clicked), this);
    gtk_box_pack_start(GTK_BOX(navigationBar), starButton, FALSE, FALSE, 0);

    // Menu hamburger (trois barres horizontales) pour les options
    GtkWidget* boutonMenu = renderingEngine->createButton("open-menu", G_CALLBACK(+[](GtkButton*, gpointer user_data) {
        auto* navigateur = static_cast<Browser*>(user_data);
        navigateur->showOptionsMenu();
    }), this);
    gtk_box_pack_start(GTK_BOX(navigationBar), boutonMenu, FALSE, FALSE, 0);

    // Ajouter la barre de navigation au container principal
    if (mainContainer && !gtk_widget_get_parent(navigationBar)) {
        gtk_box_pack_start(GTK_BOX(mainContainer), navigationBar, FALSE, FALSE, 0);
    }
}


std::string Browser::getCurrentURL() const {
    if (activeTab && activeTab->webView) {
        const gchar* uri = webkit_web_view_get_uri(activeTab->webView);
        return uri ? std::string(uri) : "";
    }
    return "";
}


void Browser::initializeFavoritesBar() {
    refreshFavoritesBar();
}

void Browser::showFavoritesMenu() {
    GtkWidget* menu = gtk_menu_new();
    for (const auto& favori : *favorites) {
        if (FavoritesJson::isFolder(favori)) {
            GtkWidget* item = gtk_menu_item_new_with_label(favori.value("name", "Dossier").c_str());
            GtkWidget* sub = gtk_menu_new();
            nlohmann::json ch = nlohmann::json::array();
            if (favori.contains("children") && favori["children"].is_array()) {
                ch = favori["children"];
            }
            populate_favorites_menu_from_children(GTK_MENU_SHELL(sub), this, ch);
            gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), sub);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        } else if (favori.contains("name") && favori.contains("url")) {
            GtkWidget* item = gtk_menu_item_new_with_label(favori["name"].get<std::string>().c_str());
            auto* data = new std::pair<Browser*, std::string>(this, favori["url"].get<std::string>());
            g_signal_connect(item, "activate", G_CALLBACK(on_menu_item_activate), data);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        }
    }
    gtk_widget_show_all(menu);
    if (favoritesBar && GTK_IS_WIDGET(favoritesBar)) {
        gtk_menu_popup_at_widget(GTK_MENU(menu), favoritesBar, GDK_GRAVITY_SOUTH, GDK_GRAVITY_NORTH, nullptr);
    } else {
        gtk_menu_popup_at_pointer(GTK_MENU(menu), nullptr);
    }
}

void Browser::showRemainingFavoritesMenu() {
    constexpr int kMaxTopLevelSlots = 10;
    GtkWidget* menu = gtk_menu_new();
    int index = 0;
    for (const auto& favori : *favorites) {
        if (index++ < kMaxTopLevelSlots) {
            continue;
        }
        if (FavoritesJson::isFolder(favori)) {
            GtkWidget* item = gtk_menu_item_new_with_label(favori.value("name", "Dossier").c_str());
            GtkWidget* sub = gtk_menu_new();
            nlohmann::json ch = nlohmann::json::array();
            if (favori.contains("children") && favori["children"].is_array()) {
                ch = favori["children"];
            }
            populate_favorites_menu_from_children(GTK_MENU_SHELL(sub), this, ch);
            gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), sub);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        } else if (favori.contains("name") && favori.contains("url")) {
            GtkWidget* item = gtk_menu_item_new_with_label(favori["name"].get<std::string>().c_str());
            auto* data = new std::pair<Browser*, std::string>(this, favori["url"].get<std::string>());
            g_signal_connect(item, "activate", G_CALLBACK(on_menu_item_activate), data);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        }
    }
    GList* kids = gtk_container_get_children(GTK_CONTAINER(menu));
    const bool empty = (kids == nullptr);
    if (kids) {
        g_list_free(kids);
    }
    if (empty) {
        GtkWidget* item = gtk_menu_item_new_with_label("(aucun)");
        gtk_widget_set_sensitive(item, FALSE);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    }
    gtk_widget_show_all(menu);
    if (favoritesBar && GTK_IS_WIDGET(favoritesBar)) {
        gtk_menu_popup_at_widget(GTK_MENU(menu), favoritesBar, GDK_GRAVITY_SOUTH, GDK_GRAVITY_NORTH, nullptr);
    } else {
        gtk_menu_popup_at_pointer(GTK_MENU(menu), nullptr);
    }
}


void Browser::showFavoritesManager() {
    if (!favoritesManager) {
        favoritesManager = std::make_unique<FavoritesManager>(favorites, [this]() { refreshFavoritesBar(); });
    }
    favoritesManager->showWindow();
}


void Browser::refreshFavoritesBar() {
    constexpr int kMaxTopLevelSlots = 10;

    if (favoritesBar) {
        if (GTK_IS_WIDGET(favoritesBar) && !gtk_widget_in_destruction(favoritesBar)) {
            GtkWidget* parent = gtk_widget_get_parent(favoritesBar);
            if (parent && GTK_IS_CONTAINER(parent) && GTK_IS_WIDGET(parent)) {
                if (GTK_IS_WIDGET(favoritesBar) && !gtk_widget_in_destruction(favoritesBar)) {
                    gtk_container_remove(GTK_CONTAINER(parent), favoritesBar);
                }
            }
            if (GTK_IS_WIDGET(favoritesBar) && !gtk_widget_in_destruction(favoritesBar)) {
                gtk_widget_destroy(favoritesBar);
            }
        }
        favoritesBar = nullptr;
    }

    favoritesBar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_widget_set_name(favoritesBar, "barre-favorites");
    gtk_widget_set_margin_start(favoritesBar, 4);
    gtk_widget_set_margin_end(favoritesBar, 4);
    gtk_widget_set_margin_top(favoritesBar, 0);
    gtk_widget_set_margin_bottom(favoritesBar, 0);

    if (!favorites || favorites->empty()) {
        GtkWidget* labelVide = gtk_label_new("");
        gtk_widget_set_opacity(labelVide, 0.0);
        gtk_box_pack_start(GTK_BOX(favoritesBar), labelVide, FALSE, FALSE, 0);
    } else {
        int compteur = 0;
        for (const auto& favori : *favorites) {
            if (compteur >= kMaxTopLevelSlots) {
                GtkWidget* boutonPlus = gtk_button_new_with_label("⋯");
                gtk_widget_set_name(boutonPlus, "button-favori-more");
                gtk_widget_set_tooltip_text(boutonPlus, "Plus de favoris");
                gtk_widget_set_margin_start(boutonPlus, 2);
                gtk_widget_set_margin_end(boutonPlus, 2);
                g_signal_connect(boutonPlus, "clicked", G_CALLBACK(+[](GtkButton*, gpointer user_data) {
                    static_cast<Browser*>(user_data)->showRemainingFavoritesMenu();
                }), this);
                gtk_box_pack_start(GTK_BOX(favoritesBar), boutonPlus, FALSE, FALSE, 0);
                break;
            }
            if (FavoritesJson::isFolder(favori)) {
                GtkWidget* folderBtn = create_folder_menu_button(this, favori);
                gtk_box_pack_start(GTK_BOX(favoritesBar), folderBtn, FALSE, FALSE, 0);
            } else if (favori.contains("url") && favori.contains("name") && favori["url"].is_string()) {
                const std::string rawName = favori["name"].get<std::string>();
                const std::string rawUrl = favori["url"].get<std::string>();
                const std::string label = truncate_favorite_label(rawName);
                GtkWidget* boutonFavori = gtk_button_new_with_label(label.c_str());
                gtk_widget_set_tooltip_text(boutonFavori, rawUrl.c_str());
                gtk_widget_set_name(boutonFavori, "button-favori");
                gtk_widget_set_margin_start(boutonFavori, 1);
                gtk_widget_set_margin_end(boutonFavori, 1);
                g_object_set_data_full(G_OBJECT(boutonFavori), "favorite-url", g_strdup(rawUrl.c_str()), g_free);
                g_object_set_data_full(G_OBJECT(boutonFavori), "favorite-name", g_strdup(rawName.c_str()), g_free);
                connect_bookmark_button_clicked(boutonFavori, this, rawUrl);
                g_signal_connect(boutonFavori, "button-press-event", G_CALLBACK(on_favoris_button_press), this);
                gtk_box_pack_start(GTK_BOX(favoritesBar), boutonFavori, FALSE, FALSE, 0);
            }
            compteur++;
        }
    }

    if (mainContainer && !gtk_widget_get_parent(favoritesBar)) {
        gtk_box_pack_start(GTK_BOX(mainContainer), favoritesBar, FALSE, FALSE, 0);
        gtk_box_reorder_child(GTK_BOX(mainContainer), favoritesBar, 2);
    }
    gtk_widget_show_all(favoritesBar);
}


void Browser::updateStarButton() {
    std::string urlActuelle = getCurrentURL();
    const bool estDejaFavori = FavoritesJson::containsUrlRecursive(*favorites, urlActuelle);

    const char* symbole = estDejaFavori ? "★" : "☆";
    gtk_button_set_label(GTK_BUTTON(starButton), symbole);
}


void Browser::initializeFavoritesPopover() {
    //favoritesPopover = gtk_popover_new(favoritesBar); // Attaché à la barre de favorites
    favoritesPopover = gtk_popover_new(navigationBar); // Attaché à la barre
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    // Champs de saisie
    favoriteNameEntry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(favoriteNameEntry), "Nom du favori");

    favoriteUrlEntry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(favoriteUrlEntry), "URL du favori");

    // Boutton de validation
    GtkWidget* boutonAjouter = gtk_button_new_with_label("Ajouter Favori");
    g_signal_connect(boutonAjouter, "clicked", G_CALLBACK(on_button_ajouter_clicked), this);

    // Ajout des éléments dans la boîte
    gtk_box_pack_start(GTK_BOX(box), favoriteNameEntry, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(box), favoriteUrlEntry, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(box), boutonAjouter, FALSE, FALSE, 5);
    
    // Ajouter le contenu au popover
    gtk_container_add(GTK_CONTAINER(favoritesPopover), box);
    // Ne pas afficher le popover automatiquement - il sera affiché uniquement quand l'utilisateur le demande
    // gtk_widget_show_all(favoritesPopover); // Retiré pour éviter l'affichage au démarrage
}


void Browser::initializeTabsBar() {
    // Nettoyer l'ancienne barre d'tabs si elle existe
    // CRITIQUE : Vérifier que tabsBar est valide avant toute manipulation
    if (tabsBar) {
        // Vérifier que c'est un widget GTK valide et qu'il n'est pas déjà en destruction
        if (GTK_IS_WIDGET(tabsBar) && !gtk_widget_in_destruction(tabsBar)) {
            // Retirer du container avant de détruire
            GtkWidget* parent = gtk_widget_get_parent(tabsBar);
            if (parent && GTK_IS_CONTAINER(parent) && GTK_IS_WIDGET(parent)) {
                // Vérifier à nouveau que le widget est toujours valide
                if (GTK_IS_WIDGET(tabsBar) && !gtk_widget_in_destruction(tabsBar)) {
                    gtk_container_remove(GTK_CONTAINER(parent), tabsBar);
                }
            }
            // Vérifier une dernière fois avant destruction
            if (GTK_IS_WIDGET(tabsBar) && !gtk_widget_in_destruction(tabsBar)) {
                gtk_widget_destroy(tabsBar);
            }
        }
        // Toujours réinitialiser le pointeur après tentative de destruction
        tabsBar = nullptr;
    }

    // Créer un ScrolledWindow pour rendre les onglets scrollables
    GtkWidget* scrolledWindow = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledWindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolledWindow), GTK_SHADOW_NONE);
    gtk_widget_set_name(scrolledWindow, "scrolled-tabs");
    
    // Créer une nouvelle barre d'tabs avec container visible
    tabsBar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_widget_set_margin_start(tabsBar, 5);
    gtk_widget_set_margin_end(tabsBar, 5);
    gtk_widget_set_margin_top(tabsBar, 5);
    gtk_widget_set_margin_bottom(tabsBar, 2);
    gtk_widget_set_name(tabsBar, "barre-tabs");
    
    // Ajouter la barre d'onglets au ScrolledWindow
    gtk_container_add(GTK_CONTAINER(scrolledWindow), tabsBar);
    
    // Bouton pour gérer les groupes (à gauche)
    GtkWidget* boutonGroupes = gtk_button_new_with_label("📁");
    gtk_widget_set_tooltip_text(boutonGroupes, "Gérer les groupes d'tabs");
    gtk_widget_set_margin_start(boutonGroupes, 2);
    gtk_widget_set_margin_end(boutonGroupes, 2);
    gtk_widget_set_name(boutonGroupes, "button-groupes");
    g_signal_connect(boutonGroupes, "clicked", G_CALLBACK(+[](GtkButton*, gpointer user_data) {
        auto* navigateur = static_cast<Browser*>(user_data);
        navigateur->showGroupsMenu();
    }), this);
    gtk_box_pack_start(GTK_BOX(tabsBar), boutonGroupes, FALSE, FALSE, 0);
    gtk_widget_show_all(boutonGroupes);
    
    // Les tabs seront ajoutés ici (au milieu)
    // Le bouton "+" sera ajouté dynamiquement après chaque onglet dans addNewTab()
    
    // Ajouter le ScrolledWindow au container principal (en haut)
    if (mainContainer && !gtk_widget_get_parent(scrolledWindow)) {
        gtk_box_pack_start(GTK_BOX(mainContainer), scrolledWindow, FALSE, FALSE, 0);
    }
    
    // Connecter le signal scroll-event pour permettre le changement d'onglet avec la molette
    g_signal_connect(scrolledWindow, "scroll-event", G_CALLBACK(+[](GtkWidget* widget, GdkEventScroll* event, gpointer user_data) -> gboolean {
        auto* navigateur = static_cast<Browser*>(user_data);
        if (navigateur->tabs.empty() || !navigateur->activeTab) return FALSE;
        
        // Trouver l'onglet actif
        int currentIndex = -1;
        for (size_t i = 0; i < navigateur->tabs.size(); ++i) {
            if (navigateur->tabs[i].tabWidget == navigateur->activeTab->tabWidget) {
                currentIndex = i;
                break;
            }
        }
        
        if (currentIndex == -1) return FALSE;
        
        // Changer d'onglet selon la direction du scroll
        if (event->direction == GDK_SCROLL_DOWN || event->direction == GDK_SCROLL_SMOOTH) {
            if (event->delta_y > 0 && currentIndex < static_cast<int>(navigateur->tabs.size()) - 1) {
                navigateur->changeActiveTab(navigateur->tabs[currentIndex + 1].tabWidget);
                return TRUE;
            }
        } else if (event->direction == GDK_SCROLL_UP || event->direction == GDK_SCROLL_SMOOTH) {
            if (event->delta_y < 0 && currentIndex > 0) {
                navigateur->changeActiveTab(navigateur->tabs[currentIndex - 1].tabWidget);
                return TRUE;
            }
        }
        
        return FALSE;
    }), this);
}

void Browser::changeTabGroup(const std::string& groupName) {
    tabsManager->changerGroupeActif(groupName);
    // Retirer le ScrolledWindow du container avant de détruire
    if (tabsBar && GTK_IS_WIDGET(tabsBar)) {
        GtkWidget* scrolledWindow = gtk_widget_get_parent(tabsBar);
        if (scrolledWindow && GTK_IS_SCROLLED_WINDOW(scrolledWindow) && 
            GTK_IS_WIDGET(scrolledWindow) && !gtk_widget_in_destruction(scrolledWindow)) {
            GtkWidget* parent = gtk_widget_get_parent(scrolledWindow);
            if (parent && GTK_IS_CONTAINER(parent)) {
                gtk_container_remove(GTK_CONTAINER(parent), scrolledWindow);
            }
            // Vérifier à nouveau avant destruction
            if (GTK_IS_WIDGET(scrolledWindow) && !gtk_widget_in_destruction(scrolledWindow)) {
                gtk_widget_destroy(scrolledWindow);
            }
        }
        tabsBar = nullptr;
    }
    initializeTabsBar();
}


void Browser::addNewTab(const std::string &url) {
    tabsManager->ajouterOnglet(tabsManager->getGroupeActif(), url);

    // Créer un container visible pour l'onglet avec style
    GtkWidget *hboxOnglet = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_widget_set_margin_start(hboxOnglet, 2);
    gtk_widget_set_margin_end(hboxOnglet, 2);
    gtk_widget_set_margin_top(hboxOnglet, 2);
    gtk_widget_set_margin_bottom(hboxOnglet, 2);
    gtk_widget_set_name(hboxOnglet, "onglet");
    
    // Label avec le titre de l'onglet (ou "Nouvel onglet" par défaut)
    GtkWidget *labelTitre = gtk_label_new("Nouvel onglet");
    gtk_label_set_ellipsize(GTK_LABEL(labelTitre), PANGO_ELLIPSIZE_END);
    gtk_widget_set_margin_start(labelTitre, 8);
    gtk_widget_set_margin_end(labelTitre, 8);
    gtk_box_pack_start(GTK_BOX(hboxOnglet), labelTitre, TRUE, TRUE, 0);

    // Bouton fermer (×) à droite
    GtkWidget *boutonFermer = gtk_button_new_with_label("×");
    gtk_widget_set_tooltip_text(boutonFermer, "Fermer l'onglet");
    gtk_widget_set_margin_start(boutonFermer, 2);
    gtk_widget_set_margin_end(boutonFermer, 2);
    g_signal_connect(boutonFermer, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer user_data) -> gboolean {
        auto* n = static_cast<Browser*>(user_data);
        // Trouver le container parent (hboxOnglet) - remonter de 1 niveau
        GtkWidget *parent = gtk_widget_get_parent(GTK_WIDGET(button));
        if (parent) {
            n->removeTab(parent);
        }
        return TRUE; // Empêcher la propagation
    }), this);
    gtk_box_pack_start(GTK_BOX(hboxOnglet), boutonFermer, FALSE, FALSE, 0);

    // Rendre l'onglet cliquable pour changer d'onglet actif
    gtk_widget_set_events(hboxOnglet, GDK_BUTTON_PRESS_MASK);
    // Définir le curseur pointer en code (GTK CSS ne supporte pas cursor)
    // Attendre que le widget soit réalisé pour définir le curseur
    g_signal_connect(hboxOnglet, "realize", G_CALLBACK(+[](GtkWidget* widget, gpointer) {
        GdkWindow* window = gtk_widget_get_window(widget);
        if (window) {
            GdkDisplay* display = gtk_widget_get_display(widget);
            if (display) {
                GdkCursor* cursor = gdk_cursor_new_from_name(display, "pointer");
                if (cursor) {
                    gdk_window_set_cursor(window, cursor);
                    g_object_unref(cursor);
                }
            }
        }
    }), nullptr);
    g_signal_connect(hboxOnglet, "button-press-event", G_CALLBACK(+[](GtkWidget* widget, GdkEventButton*, gpointer user_data) -> gboolean {
        auto* n = static_cast<Browser*>(user_data);
        if (n) {
            n->changeActiveTab(widget);
        }
        return TRUE;
    }), this);

    // Ajouter l'onglet à la barre d'tabs (après le button groupes)
    if (tabsBar && !gtk_widget_get_parent(hboxOnglet)) {
        // Retirer le bouton "+" s'il existe déjà pour le réinsérer après le nouvel onglet
        GList* children = gtk_container_get_children(GTK_CONTAINER(tabsBar));
        GtkWidget* boutonAjouterOnglet = nullptr;
        
        // Chercher le bouton "+" dans les enfants
        for (GList* iter = children; iter != nullptr; iter = iter->next) {
            GtkWidget* widget = GTK_WIDGET(iter->data);
            const gchar* name = gtk_widget_get_name(widget);
            if (name && strcmp(name, "button-ajouter-onglet") == 0) {
                boutonAjouterOnglet = widget;
                // Retirer temporairement le bouton du container
                gtk_container_remove(GTK_CONTAINER(tabsBar), widget);
                break;
            }
        }
        g_list_free(children);
        
        // Ajouter le nouvel onglet
        gtk_box_pack_start(GTK_BOX(tabsBar), hboxOnglet, FALSE, FALSE, 0);
        
        // Réinsérer le bouton "+" juste après le nouvel onglet (pas à l'extrême droite)
        if (boutonAjouterOnglet) {
            gtk_box_pack_start(GTK_BOX(tabsBar), boutonAjouterOnglet, FALSE, FALSE, 0);
        } else {
            // Créer le bouton "+" s'il n'existe pas encore
            boutonAjouterOnglet = gtk_button_new_with_label("+");
            gtk_widget_set_tooltip_text(boutonAjouterOnglet, "Ajouter un nouvel onglet");
            gtk_widget_set_margin_start(boutonAjouterOnglet, 2);
            gtk_widget_set_margin_end(boutonAjouterOnglet, 2);
            gtk_widget_set_name(boutonAjouterOnglet, "button-ajouter-onglet");
            g_signal_connect(boutonAjouterOnglet, "clicked", G_CALLBACK(on_ajouter_onglet), this);
            gtk_box_pack_start(GTK_BOX(tabsBar), boutonAjouterOnglet, FALSE, FALSE, 0);
            gtk_widget_show_all(boutonAjouterOnglet);
        }
    }
    // Normaliser l'URL
    std::string urlNormalisee = url.empty() ? "https://www.duckduckgo.com" : url;
    if (urlNormalisee.find("://") == std::string::npos) {
        urlNormalisee = "https://" + urlNormalisee;
    }
    
    // Créer la WebView (utilise le contexte par défaut)
    // Les variables d'environnement dans le Makefile désactivent l'accélération GPU
    WebKitWebView* newWebView = WEBKIT_WEB_VIEW(webkit_web_view_new());
    
    // Vérifier que la WebView est correctement créée
    if (!newWebView) {
        std::cerr << "[ERREUR] webkit_web_view_new() a retourné NULL" << std::endl;
        return;
    }
    
    // Vérifier que c'est bien une WebView valide
    if (!WEBKIT_IS_WEB_VIEW(newWebView)) {
        std::cerr << "[ERREUR] L'objet créé n'est pas une WebView valide" << std::endl;
        g_object_unref(newWebView);
        return;
    }
    
    // Vérifier que c'est un widget GTK valide
    if (!GTK_IS_WIDGET(newWebView)) {
        std::cerr << "[ERREUR] WebView n'est pas un widget GTK valide" << std::endl;
        g_object_unref(newWebView);
        return;
    }
    
    GtkWidget* webWidget = GTK_WIDGET(newWebView);
    
    // Garder la console des pages silencieuse par défaut : les warnings CSP/preload
    // viennent souvent du site ou de WebKit, pas du shell du navigateur.
    WebKitSettings* settings = webkit_web_view_get_settings(newWebView);
    if (settings) {
        webkit_settings_set_enable_write_console_messages_to_stdout(settings, isDebugLoggingEnabled());
        // Activer JavaScript (devrait être activé par défaut)
        webkit_settings_set_enable_javascript(settings, TRUE);
        // Note: webkit_settings_set_enable_plugins est déprécié et ne fait rien
    }
    
    // Donner un nom CSS à la WebView pour le debug visuel
    gtk_widget_set_name(webWidget, "webkit-webview");
    
    // Configuration de base de la WebView
    gtk_widget_set_vexpand(webWidget, TRUE);
    gtk_widget_set_hexpand(webWidget, TRUE);
    
    // CRITIQUE : S'assurer que la WebView a une taille minimale valide
    // WebKit nécessite une taille valide pour initialiser le contexte de rendu
    // Ne pas fixer de taille minimale pour permettre le redimensionnement complet
    gtk_widget_set_size_request(webWidget, 1, 1);
    
    // Créer la structure TabData AVANT d'ajouter au container
    TabData tabData;
    tabData.url = urlNormalisee;
    tabData.tabWidget = hboxOnglet;
    tabData.webView = newWebView;
    tabData.label = labelTitre;
    
    // Ajouter l'onglet à la liste
    tabs.push_back(tabData);
    
    // IMPORTANT : Ne PAS ajouter la WebView au container ici
    // Laisser changeActiveTab gérer l'ajout/retrait des WebViews
    // Cela évite d'invalider la WebView active qui pourrait être en cours d'utilisation
    
    // Juste s'assurer que la WebView n'est pas déjà dans un autre container
    GtkWidget* oldParent = gtk_widget_get_parent(webWidget);
    if (oldParent && oldParent != webContainer) {
        // Ne pas retirer ici, changeActiveTab le fera de manière sécurisée
        WEEDLYWEB_DEBUG_LOG("[DEBUG] WebView a un parent différent, sera géré par changeActiveTab");
    }
    
    // Afficher l'onglet
    gtk_widget_show_all(hboxOnglet);
    
    // Forcer l'affichage de la WebView et du container
    if (webContainer) {
        gtk_widget_show_all(webContainer);
        gtk_widget_queue_draw(webContainer);
        
        // CRITIQUE : Attendre que le container soit réalisé et obtenir sa taille
        // Utiliser un timeout pour forcer le redimensionnement après que tout soit affiché
        g_timeout_add(100, [](gpointer user_data) -> gboolean {
            auto* browser = static_cast<Browser*>(user_data);
            if (browser && browser->webContainer) {
                int containerWidth = gtk_widget_get_allocated_width(browser->webContainer);
                int containerHeight = gtk_widget_get_allocated_height(browser->webContainer);
                WEEDLYWEB_DEBUG_LOG("[DEBUG] Container allocated size: " << containerWidth << "x" << containerHeight);
                
                // Si le container a une taille valide, redimensionner toutes les WebViews
                if (containerWidth > 100 && containerHeight > 100) {
                    for (auto& tab : browser->tabs) {
                        if (tab.webView && GTK_IS_WIDGET(tab.webView)) {
                            GtkWidget* w = GTK_WIDGET(tab.webView);
                            gtk_widget_set_size_request(w, -1, -1); // Réinitialiser
                            gtk_widget_queue_resize(w);
                            gtk_widget_queue_draw(w);
                        }
                    }
                }
            }
            return FALSE; // Ne pas répéter
        }, this);
    }
    gtk_widget_show_all(webWidget);
    gtk_widget_queue_draw(webWidget);
    
    // Connecter le signal pour mettre à jour le titre de l'onglet quand la page se charge
    // IMPORTANT: Utiliser un pointeur stable (l'index dans le vector) au lieu de &tabs.back()
    // car &tabs.back() peut devenir invalide si le vector est réalloué
    if (WEBKIT_IS_WEB_VIEW(newWebView) && G_IS_OBJECT(newWebView)) {
        // Stocker l'index de l'onglet dans le vector
        size_t tabIndex = tabs.size() - 1;
        
        g_signal_connect(newWebView, "notify::title", G_CALLBACK(+[](GObject* obj, GParamSpec*, gpointer user_data) {
            auto* browser = static_cast<Browser*>(user_data);
            if (!browser) return;
            
            // Trouver l'onglet correspondant à cette WebView
            WebKitWebView* webView = WEBKIT_WEB_VIEW(obj);
            for (auto& tab : browser->tabs) {
                if (tab.webView == webView && tab.label && GTK_IS_LABEL(tab.label)) {
                    const gchar* title = webkit_web_view_get_title(tab.webView);
                    if (title) {
                        std::string titreStr(title);
                        std::string titreCourt = titreStr.length() > 20 ? titreStr.substr(0, 17) + "..." : titreStr;
                        gtk_label_set_text(GTK_LABEL(tab.label), titreCourt.c_str());
                    }
                    break;
                }
            }
        }), this);
    } else {
        std::cerr << "[ERREUR] Impossible de connecter le signal notify::title - WebView invalide" << std::endl;
    }
    
    // Changer l'onglet actif vers celui-ci (cela affichera la WebView et chargera l'URL)
    changeActiveTab(hboxOnglet);
    


    gtk_widget_show_all(tabsBar);
}

void Browser::changeActiveTab(GtkWidget* tabWidget) {
    for (auto &tab : tabs) {
        if (tab.tabWidget == tabWidget) {
            // Mettre à jour l'onglet actif
            activeTab = &tab;
            
            // Vérifier que la WebView est valide AVANT toute manipulation
            if (!tab.webView || !WEBKIT_IS_WEB_VIEW(tab.webView) || !GTK_IS_WIDGET(tab.webView)) {
                std::cerr << "[ERREUR] WebView invalide dans changeActiveTab" << std::endl;
                return;
            }
            
            GtkWidget* webWidget = GTK_WIDGET(tab.webView);
            
            // CRITIQUE : Cacher toutes les autres WebViews AVANT de manipuler le container
            // Cela évite les problèmes de rendu pendant le changement d'onglet
            for (auto &otherTab : tabs) {
                if (otherTab.webView && GTK_IS_WIDGET(otherTab.webView) && &otherTab != &tab) {
                    gtk_widget_hide(GTK_WIDGET(otherTab.webView));
                }
            }
            
            // Retirer uniquement les autres WebViews du container, PAS celle qu'on veut afficher
            // Faire cela APRÈS avoir caché les autres pour éviter les problèmes de rendu
            if (webContainer) {
                GList* children = gtk_container_get_children(GTK_CONTAINER(webContainer));
                GList* toRemove = nullptr;
                
                // D'abord, collecter les WebViews à retirer (ne pas les retirer pendant l'itération)
                for (GList* iter = children; iter != nullptr; iter = iter->next) {
                    GtkWidget* child = GTK_WIDGET(iter->data);
                    // Ne retirer que les WebViews qui ne sont pas celle qu'on veut afficher
                    if (GTK_IS_WIDGET(child) && child != webWidget && WEBKIT_IS_WEB_VIEW(child)) {
                        // Vérifier que ce n'est pas une WebView active en cours d'utilisation
                        bool isActive = false;
                        for (auto &otherTab : tabs) {
                            if (otherTab.webView && GTK_WIDGET(otherTab.webView) == child && &otherTab == activeTab) {
                                isActive = true;
                                break;
                            }
                        }
                        if (!isActive) {
                            toRemove = g_list_prepend(toRemove, child);
                        }
                    }
                }
                g_list_free(children);
                
                // Maintenant retirer les WebViews collectées
                // CRITIQUE : Vérifier que chaque widget est valide avant de le retirer
                for (GList* iter = toRemove; iter != nullptr; iter = iter->next) {
                    GtkWidget* child = GTK_WIDGET(iter->data);
                    // Vérifier que le widget est valide, qu'il a le bon parent, et qu'il n'est pas en destruction
                    if (GTK_IS_WIDGET(child) && 
                        !gtk_widget_in_destruction(child) &&
                        gtk_widget_get_parent(child) == webContainer) {
                        // Vérifier à nouveau après avoir obtenu le parent
                        if (GTK_IS_WIDGET(child) && !gtk_widget_in_destruction(child)) {
                            gtk_container_remove(GTK_CONTAINER(webContainer), child);
                        }
                    }
                }
                g_list_free(toRemove);
            }
            
            // Afficher la WebView de l'onglet actif
            // Vérifier à nouveau que la WebView est toujours valide après les manipulations
            if (!WEBKIT_IS_WEB_VIEW(tab.webView) || !GTK_IS_WIDGET(tab.webView)) {
                std::cerr << "[ERREUR] WebView devenue invalide après manipulation du container" << std::endl;
                return;
            }
            
            // 1. S'assurer que le container web est visible et expansible
            if (webContainer) {
                gtk_widget_show_all(webContainer);
                gtk_widget_set_visible(webContainer, TRUE);
                gtk_widget_set_vexpand(webContainer, TRUE);
                gtk_widget_set_hexpand(webContainer, TRUE);
            }
            
            // 2. CRITIQUE : Ajouter UNIQUEMENT la WebView active au container
            // (on a déjà retiré toutes les autres ci-dessus)
            if (webContainer) {
                // Vérifier où se trouve la WebView actuellement
                GtkWidget* currentParent = gtk_widget_get_parent(webWidget);
                
                if (!currentParent) {
                    // WebView n'a pas de parent, l'ajouter au container
                    gtk_box_pack_start(GTK_BOX(webContainer), webWidget, TRUE, TRUE, 0);
                    WEEDLYWEB_DEBUG_LOG("[DEBUG] WebView ajoutée au container lors du changement d'onglet");
                } else if (currentParent == webContainer) {
                    // WebView est déjà dans le bon container, juste s'assurer qu'elle est visible
                    WEEDLYWEB_DEBUG_LOG("[DEBUG] WebView déjà dans le container, forcer l'affichage...");
                    // NE PAS retirer/réajouter car cela peut invalider la WebView
                    // Juste forcer l'affichage et le redessinage
                } else {
                    // WebView est dans un autre container, la déplacer
                    WEEDLYWEB_DEBUG_LOG("[DEBUG] WebView dans un autre container, déplacement...");
                    // Vérifier que le parent et le widget sont valides avant retrait
                    if (GTK_IS_CONTAINER(currentParent) && GTK_IS_WIDGET(webWidget) && 
                        !gtk_widget_in_destruction(webWidget) && !gtk_widget_in_destruction(currentParent)) {
                        gtk_container_remove(GTK_CONTAINER(currentParent), webWidget);
                        
                        // Vérifier que la WebView est toujours valide après retrait
                        if (!WEBKIT_IS_WEB_VIEW(tab.webView) || !GTK_IS_WIDGET(tab.webView) || 
                            gtk_widget_in_destruction(webWidget)) {
                            std::cerr << "[ERREUR] WebView devenue invalide après retrait du parent" << std::endl;
                            return;
                        }
                        
                        // Vérifier à nouveau avant d'ajouter au nouveau container
                        if (GTK_IS_WIDGET(webWidget) && !gtk_widget_in_destruction(webWidget)) {
                            gtk_box_pack_start(GTK_BOX(webContainer), webWidget, TRUE, TRUE, 0);
                        } else {
                            std::cerr << "[ERREUR] WebView invalide avant ajout au container" << std::endl;
                            return;
                        }
                    } else {
                        std::cerr << "[ERREUR] Parent ou WebView invalide avant déplacement" << std::endl;
                        return;
                    }
                }
            }
            
            // 3. Afficher la WebView IMMÉDIATEMENT et forcer sa visibilité
            // CRITIQUE : Toujours forcer l'affichage même si la WebView était déjà visible
            gtk_widget_show_all(webWidget);
            gtk_widget_set_visible(webWidget, TRUE);
            gtk_widget_set_vexpand(webWidget, TRUE);
            gtk_widget_set_hexpand(webWidget, TRUE);
            
            // Forcer le redessinage immédiat
            gtk_widget_queue_resize(webWidget);
            gtk_widget_queue_draw(webWidget);
            
            // Forcer la réalisation si nécessaire
            if (!gtk_widget_get_realized(webWidget)) {
                gtk_widget_realize(webWidget);
            }
            
            WEEDLYWEB_DEBUG_LOG("[DEBUG] WebView shown: " << (gtk_widget_get_visible(webWidget) ? "YES" : "NO"));
            WEEDLYWEB_DEBUG_LOG("[DEBUG] WebView parent: " << (gtk_widget_get_parent(webWidget) ? "YES" : "NO"));
            WEEDLYWEB_DEBUG_LOG("[DEBUG] WebView realized: " << (gtk_widget_get_realized(webWidget) ? "YES" : "NO"));
            
            // 4. Charger l'URL immédiatement
            // Vérifier que la WebView est valide avant d'utiliser
            if (!WEBKIT_IS_WEB_VIEW(tab.webView)) {
                std::cerr << "[ERREUR] WebView invalide avant chargement URL" << std::endl;
                return;
            }
            const gchar* currentUri = webkit_web_view_get_uri(tab.webView);
            std::string currentUrl = currentUri ? std::string(currentUri) : "";
            
            // CRITIQUE : Toujours recharger l'URL si elle est différente ou vide
            // Même si l'URL semble chargée, forcer le rechargement pour s'assurer que la page s'affiche
            bool needsReload = currentUrl.empty() || currentUrl != tab.url;
            
            if (needsReload) {
                WEEDLYWEB_DEBUG_LOG("[DEBUG] ========== LOADING URL ==========");
                WEEDLYWEB_DEBUG_LOG("[DEBUG] Target URL: " << tab.url);
                WEEDLYWEB_DEBUG_LOG("[DEBUG] Current URL: " << currentUrl);
                WEEDLYWEB_DEBUG_LOG("[DEBUG] WebView realized: " << (gtk_widget_get_realized(webWidget) ? "YES" : "NO"));
                WEEDLYWEB_DEBUG_LOG("[DEBUG] WebView visible: " << (gtk_widget_get_visible(webWidget) ? "YES" : "NO"));
                WEEDLYWEB_DEBUG_LOG("[DEBUG] WebView parent: " << (gtk_widget_get_parent(webWidget) ? "YES" : "NO"));
                
                // Obtenir la taille allouée
                int width = 0, height = 0;
                if (gtk_widget_get_realized(webWidget)) {
                    width = gtk_widget_get_allocated_width(webWidget);
                    height = gtk_widget_get_allocated_height(webWidget);
                    WEEDLYWEB_DEBUG_LOG("[DEBUG] WebView size: " << width << "x" << height);
                }
                
                // Vérifier une dernière fois que la WebView est valide avant de charger
                if (!WEBKIT_IS_WEB_VIEW(tab.webView)) {
                    std::cerr << "[ERREUR] WebView invalide avant webkit_web_view_load_uri" << std::endl;
                    return;
                }
                
                WEEDLYWEB_DEBUG_LOG("[DEBUG] Calling webkit_web_view_load_uri...");
                webkit_web_view_load_uri(tab.webView, tab.url.c_str());
                
                // Vérifier l'URI après chargement
                const gchar* loadedUri = webkit_web_view_get_uri(tab.webView);
                WEEDLYWEB_DEBUG_LOG("[DEBUG] URI after load_uri: " << (loadedUri ? loadedUri : "NULL"));
                
                // Forcer le redessinage après le chargement
                gtk_widget_queue_draw(webWidget);
                if (webContainer) {
                    gtk_widget_queue_draw(webContainer);
                }
                WEEDLYWEB_DEBUG_LOG("[DEBUG] ==================================");
            } else {
                WEEDLYWEB_DEBUG_LOG("[DEBUG] URL already loaded: " << currentUrl);
                // Même si l'URL est déjà chargée, forcer le redessinage et le rechargement visuel
                // Cela garantit que la page s'affiche correctement quand on revient à l'onglet
                gtk_widget_queue_resize(webWidget);
                gtk_widget_queue_draw(webWidget);
                if (webContainer) {
                    gtk_widget_queue_resize(webContainer);
                    gtk_widget_queue_draw(webContainer);
                }
                
                // Forcer un rechargement visuel même si l'URL est la même
                // Cela évite les problèmes d'affichage quand on revient à un onglet
                gtk_widget_show_all(webWidget);
                gtk_widget_set_visible(webWidget, TRUE);
            }
            
            // Forcer le traitement des événements pour s'assurer que tout est affiché
            int iterations = 0;
            while (gtk_events_pending() && iterations < 10) {
                gtk_main_iteration_do(FALSE);
                iterations++;
            }
            
            // 3. Connecter les signaux de chargement UNE SEULE FOIS
            // Vérifier que la WebView est valide avant de connecter les signaux
            if (!WEBKIT_IS_WEB_VIEW(tab.webView) || !G_IS_OBJECT(tab.webView)) {
                std::cerr << "[ERREUR] WebView invalide, impossible de connecter les signaux" << std::endl;
                return;
            }
            
            // Utiliser un set statique pour éviter de connecter les signaux plusieurs fois
            // Note: Les WebViews sont gérées par GTK et ne doivent pas être supprimées manuellement
            // Le set est nettoyé automatiquement quand les WebViews sont détruites par GTK
            static std::set<WebKitWebView*> connectedViews;
            
            // Vérifier que la WebView est toujours valide avant de l'ajouter au set
            if (!WEBKIT_IS_WEB_VIEW(tab.webView)) {
                std::cerr << "[ERREUR] WebView invalide, impossible de connecter les signaux" << std::endl;
                return;
            }
            
            if (connectedViews.find(tab.webView) == connectedViews.end()) {
                connectedViews.insert(tab.webView);
                    
                    // Handler pour load-changed
                    g_signal_connect(tab.webView, "load-changed", G_CALLBACK(+[](WebKitWebView* web_view, WebKitLoadEvent load_event, gpointer) {
                        const gchar* uri = webkit_web_view_get_uri(web_view);
                        WEEDLYWEB_DEBUG_LOG("[DEBUG] Load event: " << load_event << " for URI: " << (uri ? uri : "NULL"));
                        
                        if (load_event == WEBKIT_LOAD_STARTED) {
                            WEEDLYWEB_DEBUG_LOG("[DEBUG] Load started, showing spinner");
                            browser_set_loading_state(true);
                        } else if (load_event == WEBKIT_LOAD_COMMITTED) {
                            WEEDLYWEB_DEBUG_LOG("[DEBUG] Load committed");
                        } else if (load_event == WEBKIT_LOAD_FINISHED) {
                            WEEDLYWEB_DEBUG_LOG("[DEBUG] Load finished, hiding spinner");
                            browser_set_loading_state(false);
                            
                            // CRITIQUE : Forcer l'affichage après chargement
                            GtkWidget* w = GTK_WIDGET(web_view);
                            gtk_widget_show_all(w);
                            gtk_widget_set_visible(w, TRUE);
                            gtk_widget_queue_draw(w);
                            
                            if (isDebugLoggingEnabled()) {
                                const gchar* js =
                                    "console.log('=== DIAGNOSTIC WEBVIEW ===');"
                                    "console.log('Document ready: ' + document.readyState);"
                                    "console.log('URL: ' + window.location.href);"
                                    "console.log('Title: ' + document.title);"
                                    "console.log('Body exists: ' + (document.body !== null));"
                                    "console.log('Body innerHTML length: ' + (document.body ? document.body.innerHTML.length : 0));"
                                    "console.log('Window width: ' + window.innerWidth);"
                                    "console.log('Window height: ' + window.innerHeight);";

                                webkit_web_view_evaluate_javascript(web_view, js, -1, nullptr, nullptr, nullptr,
                                    [](GObject*, GAsyncResult*, gpointer) {
                                        WEEDLYWEB_DEBUG_LOG("[DEBUG] JavaScript diagnostic executed");
                                    }, nullptr);
                            }
                            
                            // CRITIQUE : Forcer le redimensionnement et le rendu après chargement
                            // (w est déjà déclaré plus haut)
                            
                            // Obtenir la taille du container parent (qui devrait être webContainer)
                            GtkWidget* container = gtk_widget_get_parent(w);
                            if (container) {
                                int containerWidth = gtk_widget_get_allocated_width(container);
                                int containerHeight = gtk_widget_get_allocated_height(container);
                                WEEDLYWEB_DEBUG_LOG("[DEBUG] Container size after load: " << containerWidth << "x" << containerHeight);
                                
                                // Réinitialiser la taille de la WebView pour permettre l'expansion
                                if (containerWidth > 100 && containerHeight > 100) {
                                    gtk_widget_set_size_request(w, -1, -1); // -1 = utiliser la taille naturelle
                                    WEEDLYWEB_DEBUG_LOG("[DEBUG] Reset WebView size request to natural size");
                                }
                                
                                // Forcer le redessinage du container aussi
                                gtk_widget_queue_resize(container);
                                gtk_widget_queue_draw(container);
                            }
                            
                            // Forcer le redessinage complet de la WebView
                            gtk_widget_queue_resize(w);
                            gtk_widget_queue_draw(w);
                            
                            // Forcer le traitement des événements pour le rendu
                            while (gtk_events_pending()) {
                                gtk_main_iteration_do(FALSE);
                            }
                            
                            // Vérifier la taille finale
                            int finalWidth = gtk_widget_get_allocated_width(w);
                            int finalHeight = gtk_widget_get_allocated_height(w);
                            WEEDLYWEB_DEBUG_LOG("[DEBUG] Final WebView size: " << finalWidth << "x" << finalHeight);
                            
                            // Si la taille est toujours 1x1, forcer un redimensionnement avec un délai
                            if (finalWidth <= 1 || finalHeight <= 1) {
                                std::cerr << "[WARNING] WebView size is still too small, scheduling resize..." << std::endl;
                                g_timeout_add(200, [](gpointer user_data) -> gboolean {
                                    GtkWidget* widget = static_cast<GtkWidget*>(user_data);
                                    if (widget && GTK_IS_WIDGET(widget)) {
                                        gtk_widget_queue_resize(widget);
                                        gtk_widget_queue_draw(widget);
                                        WEEDLYWEB_DEBUG_LOG("[DEBUG] Forced resize after timeout");
                                    }
                                    return FALSE;
                                }, w);
                            }
                        }
                    }), nullptr);
                    
                    // Handler pour load-failed (erreurs de chargement)
                    g_signal_connect(tab.webView, "load-failed", G_CALLBACK(+[](WebKitWebView* web_view, WebKitLoadEvent load_event, const gchar* failing_uri, GError* error, gpointer) {
                        std::cerr << "[ERROR] ========== LOAD FAILED ==========" << std::endl;
                        std::cerr << "[ERROR] Event: " << load_event << std::endl;
                        std::cerr << "[ERROR] URI: " << (failing_uri ? failing_uri : "NULL") << std::endl;
                        if (error) {
                            std::cerr << "[ERROR] Error code: " << error->code << std::endl;
                            std::cerr << "[ERROR] Error domain: " << g_quark_to_string(error->domain) << std::endl;
                            std::cerr << "[ERROR] Error message: " << error->message << std::endl;
                        }
                        browser_set_loading_state(false);
                        std::cerr << "[ERROR] ==================================" << std::endl;
                    }), nullptr);
            }
            
            // 5. Forcer le rendu et l'affichage final
            gtk_widget_queue_resize(webWidget);
            gtk_widget_queue_draw(webWidget);
            if (webContainer) {
                gtk_widget_queue_resize(webContainer);
                gtk_widget_queue_draw(webContainer);
            }
            
            // S'assurer que la WebView est visible une dernière fois
            gtk_widget_show_all(webWidget);
            gtk_widget_set_visible(webWidget, TRUE);
            
            // Mettre à jour la barre d'URL
            if (urlBar) {
                gtk_entry_set_text(GTK_ENTRY(urlBar), tab.url.c_str());
            }
            
            // Mettre à jour le bouton étoile
            updateStarButton();
            
            // Mettre en surbrillance l'onglet actif
            highlight(tab.tabWidget);
            
            // Forcer un dernier traitement des événements
            iterations = 0;
            while (gtk_events_pending() && iterations < 5) {
                gtk_main_iteration_do(FALSE);
                iterations++;
            }
            
            WEEDLYWEB_DEBUG_LOG("[DEBUG] Changement d'onglet terminé - WebView visible: " << (gtk_widget_get_visible(webWidget) ? "YES" : "NO") << ", parent: " << (gtk_widget_get_parent(webWidget) ? "YES" : "NO"));
            return;
        }
    }
}



void Browser::executeScriptInActiveTab(const std::string& script) {
    if (activeTab && activeTab->webView) {
        scriptEngine->executerScript(activeTab->webView, script);
    }
}

void Browser::removeFavorite(GtkWidget* widget) {
    const char* storedName = static_cast<const char*>(g_object_get_data(G_OBJECT(widget), "favorite-name"));
    const gchar* favoriteName = storedName ? storedName : gtk_button_get_label(GTK_BUTTON(widget));
    favoritesManager->removeFavorite(favoriteName ? favoriteName : "");
    refreshFavoritesBar();  // Mise à jour visuelle
}


void Browser::removeTab(GtkWidget *tabWidget) {
    // Trouver l'onglet à supprimer
    auto it = std::find_if(tabs.begin(), tabs.end(), [tabWidget](const auto &tab) {
        return tab.tabWidget == tabWidget;
    });

    if (it != tabs.end()) {
        TabData& tab = *it;
        
        // Nettoyer la WebView
        if (tab.webView && WEBKIT_IS_WEB_VIEW(tab.webView)) {
            // Déconnecter les signaux avant de retirer
            g_signal_handlers_disconnect_matched(tab.webView, G_SIGNAL_MATCH_DATA, 0, 0, nullptr, nullptr, this);
            
            // Retirer du container
            GtkWidget* webWidget = GTK_WIDGET(tab.webView);
            GtkWidget* parent = gtk_widget_get_parent(webWidget);
            if (parent && GTK_IS_CONTAINER(parent)) {
                gtk_container_remove(GTK_CONTAINER(parent), webWidget);
            }
            
            // Nettoyer le set statique des WebViews connectées
            // Accéder au même set statique utilisé dans changeActiveTab
            {
                static std::set<WebKitWebView*> connectedViews;
                connectedViews.erase(tab.webView);
            }
            
            // Libérer la référence (GTK gère la durée de vie des widgets)
            // Note: Ne pas utiliser g_object_unref ou g_clear_object car GTK gère automatiquement
            // la durée de vie des widgets quand leur parent est détruit
            tab.webView = nullptr;
        }

        // Supprimer le widget de l'onglet
        // CRITIQUE : Vérifier plusieurs fois que le widget est valide avant destruction
        if (tab.tabWidget) {
            // Vérifier que c'est bien un widget GTK valide
            if (GTK_IS_WIDGET(tab.tabWidget)) {
                // Vérifier qu'il n'est pas déjà en cours de destruction
                if (!gtk_widget_in_destruction(tab.tabWidget)) {
                    // Retirer du parent AVANT de détruire
                    GtkWidget* parent = gtk_widget_get_parent(tab.tabWidget);
                    if (parent && GTK_IS_CONTAINER(parent) && GTK_IS_WIDGET(parent)) {
                        // Vérifier à nouveau que le widget est toujours valide
                        if (GTK_IS_WIDGET(tab.tabWidget) && !gtk_widget_in_destruction(tab.tabWidget)) {
                            gtk_container_remove(GTK_CONTAINER(parent), tab.tabWidget);
                        }
                    }
                    
                    // Vérifier une dernière fois avant destruction
                    if (GTK_IS_WIDGET(tab.tabWidget) && !gtk_widget_in_destruction(tab.tabWidget)) {
                        gtk_widget_destroy(tab.tabWidget);
                    }
                }
            }
            // Réinitialiser le pointeur après destruction
            tab.tabWidget = nullptr;
        }
        
        // Mettre à jour l'onglet actif avant de supprimer
        bool wasActive = (activeTab == &(*it));
        
        // Sauvegarder le pointeur vers le widget de l'onglet suivant (si disponible)
        GtkWidget* nextTabWidget = nullptr;
        if (wasActive && tabs.size() > 1) {
            // Trouver l'onglet suivant ou précédent
            auto nextIt = std::next(it);
            if (nextIt != tabs.end()) {
                nextTabWidget = nextIt->tabWidget;
            } else {
                // Prendre l'onglet précédent
                if (it != tabs.begin()) {
                    auto prevIt = std::prev(it);
                    nextTabWidget = prevIt->tabWidget;
                }
            }
        }
        
        // Réinitialiser activeTab AVANT de supprimer de la liste
        if (wasActive) {
            activeTab = nullptr;
        }
        
        // Supprimer l'onglet de la liste
        tabs.erase(it);
        
        // Si aucun onglet n'est présent, fermer l'application
        if (tabs.empty()) {
            closeApplication();
        } else {
            // Activer un onglet restant de manière sécurisée
            if (nextTabWidget && GTK_IS_WIDGET(nextTabWidget) && !gtk_widget_in_destruction(nextTabWidget)) {
                // Vérifier que l'onglet existe toujours dans la liste
                bool tabExists = false;
                for (const auto& remainingTab : tabs) {
                    if (remainingTab.tabWidget == nextTabWidget) {
                        tabExists = true;
                        break;
                    }
                }
                if (tabExists) {
                    changeActiveTab(nextTabWidget);
                } else if (!tabs.empty()) {
                    // Fallback : prendre le premier onglet disponible
                    changeActiveTab(tabs.front().tabWidget);
                }
            } else if (!tabs.empty()) {
                // Fallback : prendre le premier onglet disponible
                changeActiveTab(tabs.front().tabWidget);
            }
        }
    }
}

void Browser::highlight(GtkWidget *tabWidget) {
    if (!tabWidget) return;
    
    // Parcourir tous les tabs et gérer les classes CSS
    for (auto &tab : tabs) {
        if (tab.tabWidget && GTK_IS_WIDGET(tab.tabWidget)) {
            GtkStyleContext* context = gtk_widget_get_style_context(tab.tabWidget);
            if (tab.tabWidget == tabWidget) {
                // Ajouter la classe "onglet-actif" à l'onglet sélectionné
                gtk_style_context_add_class(context, "onglet-actif");
            } else {
                // Retirer la classe "onglet-actif" des autres tabs
                gtk_style_context_remove_class(context, "onglet-actif");
            }
        }
    }
}

void Browser::loadURL(const std::string& url) {
    if (activeTab && activeTab->webView) {
        std::string urlNormalisee = url;
        if (urlNormalisee.find("://") == std::string::npos) {
            urlNormalisee = "https://" + urlNormalisee;
        }
        webkit_web_view_load_uri(activeTab->webView, urlNormalisee.c_str());
        activeTab->url = urlNormalisee;
        history.push_back(urlNormalisee);
    }

    if (urlBar) {
        gtk_entry_set_text(GTK_ENTRY(urlBar), url.c_str());
    }
    
    updateStarButton();
    memoryManager->optimiserMemoire();
    
    // Afficher les statistiques de mémoire périodiquement (optionnel, peut être désactivé)
    // memoryManager->afficherStatistiquesMemoire();
}

void Browser::showMessage(const std::string& message) {
    std::cout << "Message : " << message << std::endl;
}


void Browser::loadConfiguration() {
    std::string chemin = FileManager::configJSONPath();
    nlohmann::json config = FileManager::readJSON(chemin);
    
    std::string cheminFavoris = FileManager::favoritesJSONPath();
    nlohmann::json favorisJson = FileManager::readJSON(cheminFavoris);

    if (favorisJson.is_null() || favorisJson.empty()) {
        std::cerr << "Aucun favori trouvé, initialisation avec un favori par défaut." << std::endl;
        favorisJson = nlohmann::json::array({
            {{"name", "DuckDuckGo"}, {"url", "https://www.duckduckgo.com"}, {"tag", "Recherche"}}
        });
        FileManager::writeJSON(cheminFavoris, favorisJson);
    }
    
    // Assigner les favorites au membre de la classe
    *favorites = favorisJson;
    
    if (config.is_null() || config.empty()) {
        std::cerr << "Fichier de configuration non trouvé ou vide. Création d'une configuration par défaut." << std::endl;
        config["homepage"] = "https://www.google.fr";
        FileManager::writeJSON(chemin, config);
    }

    if (config.contains("tabs")) {
        for (const auto& onglet : config["tabs"]) {
            if (onglet.contains("url")) {
                addNewTab(onglet["url"]);
            }
        }
    }
    
    homepage = config.value("homepage", "https://www.google.fr");
    std::cout << "Page d'accueil définie sur : " << homepage << std::endl;
}


void Browser::saveConfiguration() {
    nlohmann::json config;
    config["homepage"] = homepage;
    FileManager::writeJSON(FileManager::configJSONPath(), config);
    
    FileManager::writeJSON(FileManager::favoritesJSONPath(), *favorites);
    std::cout << "Favoris sauvegardés automatiquement !" << std::endl;
    
    nlohmann::json ongletsJson = nlohmann::json::array();
    for (const auto& tab : tabs) {
        ongletsJson.push_back({{"url", tab.url}});
    }
    config["tabs"] = ongletsJson;
}

void Browser::addFavorite(const std::string& nom, const std::string& url, const std::string& tag) {
    favoritesManager->addFavorite(nom, url, tag);
    refreshFavoritesBar();
}



void Browser::showOptionsMenu() {
    GtkWidget* menu = gtk_menu_new();
    
    // Option : Gestionnaire de favorites
    GtkWidget* itemFavoris = gtk_menu_item_new_with_label("Gestionnaire de Favoris");
    g_signal_connect(itemFavoris, "activate", G_CALLBACK(+[](GtkWidget*, gpointer user_data) {
        auto* navigateur = static_cast<Browser*>(user_data);
        navigateur->showFavoritesManager();
    }), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), itemFavoris);
    
    // Séparateur
    GtkWidget* separator1 = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), separator1);
    
    // Option : Paramètres
    GtkWidget* itemParametres = gtk_menu_item_new_with_label("⚙️ Paramètres");
    g_signal_connect(itemParametres, "activate", G_CALLBACK(+[](GtkWidget*, gpointer user_data) {
        auto* navigateur = static_cast<Browser*>(user_data);
        navigateur->showSettings();
    }), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), itemParametres);
    
    // Option : Aide
    GtkWidget* itemAide = gtk_menu_item_new_with_label("❓ Aide");
    g_signal_connect(itemAide, "activate", G_CALLBACK(+[](GtkWidget*, gpointer user_data) {
        auto* navigateur = static_cast<Browser*>(user_data);
        navigateur->showHelp();
    }), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), itemAide);
    
    // Option : À propos
    GtkWidget* itemAPropos = gtk_menu_item_new_with_label("ℹ️ À propos");
    g_signal_connect(itemAPropos, "activate", G_CALLBACK(+[](GtkWidget*, gpointer) {
        GtkWidget* dialog = gtk_message_dialog_new(
            nullptr,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_INFO,
            GTK_BUTTONS_OK,
            "WeedlyWeb\n\nNavigateur web moderne basé sur WebKit2GTK\nVersion 1.0"
        );
        gtk_dialog_run(GTK_DIALOG(dialog));
        if (dialog && GTK_IS_WIDGET(dialog) && !gtk_widget_in_destruction(dialog)) {
            gtk_widget_destroy(dialog);
        }
    }), nullptr);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), itemAPropos);
    
    // Séparateur
    GtkWidget* separator2 = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), separator2);
    
    // Option : Quitter
    GtkWidget* itemQuitter = gtk_menu_item_new_with_label("🚪 Quitter");
    g_signal_connect(itemQuitter, "activate", G_CALLBACK(+[](GtkWidget*, gpointer user_data) {
        auto* navigateur = static_cast<Browser*>(user_data);
        navigateur->closeApplication();
    }), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), itemQuitter);
    
    gtk_widget_show_all(menu);
    
    // Trouver le button menu pour positionner le popup
    // On cherche le dernier button ajouté (qui devrait être le menu hamburger)
    GList* children = gtk_container_get_children(GTK_CONTAINER(navigationBar));
    GtkWidget* boutonMenu = nullptr;
    // Prendre le dernier enfant qui est un button (le menu hamburger est ajouté en dernier)
    if (children) {
        GList* last = g_list_last(children);
        if (last && GTK_IS_BUTTON(GTK_WIDGET(last->data))) {
            boutonMenu = GTK_WIDGET(last->data);
        }
    }
    g_list_free(children);
    
    if (boutonMenu) {
        gtk_menu_popup_at_widget(GTK_MENU(menu), boutonMenu, GDK_GRAVITY_SOUTH_WEST, GDK_GRAVITY_NORTH_WEST, nullptr);
    } else {
        gtk_menu_popup_at_pointer(GTK_MENU(menu), nullptr);
    }
}

void Browser::showGroupsMenu() {
    GtkWidget* menu = gtk_menu_new();
    
    // Option : Créer un nouveau groupe
    GtkWidget* itemNouveauGroupe = gtk_menu_item_new_with_label("➕ Créer un nouveau groupe");
    g_signal_connect(itemNouveauGroupe, "activate", G_CALLBACK(+[](GtkWidget*, gpointer user_data) {
        auto* navigateur = static_cast<Browser*>(user_data);
        // Créer une boîte de dialogue pour le nom du groupe
        GtkWidget* dialog = gtk_dialog_new_with_buttons(
            "Nouveau groupe",
            nullptr,
            GTK_DIALOG_MODAL,
            "Annuler", GTK_RESPONSE_CANCEL,
            "Créer", GTK_RESPONSE_ACCEPT,
            nullptr
        );
        GtkWidget* entry = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Nom du groupe");
        gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), entry);
        gtk_widget_show_all(dialog);
        
        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
            const gchar* groupName = gtk_entry_get_text(GTK_ENTRY(entry));
            if (groupName && *groupName) {
                navigateur->getTabsManager()->ajouterGroupe(groupName);
                navigateur->changeTabGroup(groupName);
            }
        }
        if (dialog && GTK_IS_WIDGET(dialog) && !gtk_widget_in_destruction(dialog)) {
            gtk_widget_destroy(dialog);
        }
    }), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), itemNouveauGroupe);
    
    // Séparateur
    GtkWidget* separator = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), separator);
    
    // Lister les groupes existants
    for (const auto& groupe : tabsManager->getGroupes()) {
        GtkWidget* itemGroupe = gtk_menu_item_new_with_label(groupe.c_str());
        g_signal_connect(itemGroupe, "activate", G_CALLBACK(+[](GtkWidget* item, gpointer user_data) {
            auto* navigateur = static_cast<Browser*>(user_data);
            const gchar* groupName = gtk_menu_item_get_label(GTK_MENU_ITEM(item));
            navigateur->changeTabGroup(groupName);
        }), this);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), itemGroupe);
    }
    
    gtk_widget_show_all(menu);
    
    // Trouver le button groupes pour positionner le popup (premier enfant, à gauche)
    GList* children = gtk_container_get_children(GTK_CONTAINER(tabsBar));
    GtkWidget* boutonGroupes = nullptr;
    if (children) {
        // Prendre le premier enfant (le bouton groupes est le premier)
        if (children->data && GTK_IS_BUTTON(GTK_WIDGET(children->data))) {
            boutonGroupes = GTK_WIDGET(children->data);
        }
    }
    g_list_free(children);
    
    if (boutonGroupes) {
        gtk_menu_popup_at_widget(GTK_MENU(menu), boutonGroupes, GDK_GRAVITY_SOUTH_WEST, GDK_GRAVITY_NORTH_WEST, nullptr);
    } else {
        gtk_menu_popup_at_pointer(GTK_MENU(menu), nullptr);
    }
}

void Browser::showSettings() {
    std::string cheminParametres = FileManager::cheminParametresHTML();
    std::string urlParametres = "file://" + cheminParametres;
    
    // Vérifier que le fichier existe
    std::ifstream fichier(cheminParametres);
    if (!fichier.good()) {
        std::cerr << "[ERREUR] Le fichier de paramètres n'existe pas: " << cheminParametres << std::endl;
        // Créer un contenu HTML minimal si le fichier n'existe pas
        GtkWidget* dialog = gtk_message_dialog_new(
            nullptr,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_INFO,
            GTK_BUTTONS_OK,
            "Page de paramètres\n\nLe fichier de paramètres sera disponible prochainement."
        );
        gtk_dialog_run(GTK_DIALOG(dialog));
        if (dialog && GTK_IS_WIDGET(dialog) && !gtk_widget_in_destruction(dialog)) {
            gtk_widget_destroy(dialog);
        }
        return;
    }
    fichier.close();
    
    // Charger dans l'onglet actif ou créer un nouvel onglet
    if (activeTab && activeTab->webView) {
        webkit_web_view_load_uri(activeTab->webView, urlParametres.c_str());
        activeTab->url = urlParametres;
        if (urlBar) {
            gtk_entry_set_text(GTK_ENTRY(urlBar), urlParametres.c_str());
        }
    } else {
        addNewTab(urlParametres);
    }
}

void Browser::showHelp() {
    std::string cheminHelp = FileManager::cheminHelpHTML();
    std::string urlHelp = "file://" + cheminHelp;
    
    // Vérifier que le fichier existe
    std::ifstream fichier(cheminHelp);
    if (!fichier.good()) {
        std::cerr << "[ERREUR] Le fichier d'aide n'existe pas: " << cheminHelp << std::endl;
        // Afficher un message d'erreur si le fichier n'existe pas
        GtkWidget* dialog = gtk_message_dialog_new(
            nullptr,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_OK,
            "Page d'aide\n\nLe fichier d'aide n'a pas pu être chargé."
        );
        gtk_dialog_run(GTK_DIALOG(dialog));
        if (dialog && GTK_IS_WIDGET(dialog) && !gtk_widget_in_destruction(dialog)) {
            gtk_widget_destroy(dialog);
        }
        return;
    }
    fichier.close();
    
    // Charger dans l'onglet actif ou créer un nouvel onglet
    if (activeTab && activeTab->webView) {
        webkit_web_view_load_uri(activeTab->webView, urlHelp.c_str());
        activeTab->url = urlHelp;
        if (urlBar) {
            gtk_entry_set_text(GTK_ENTRY(urlBar), urlHelp.c_str());
        }
    } else {
        addNewTab(urlHelp);
    }
}


void Browser::configureKeyboardShortcuts() {
    g_signal_connect(window, "key-press-event", G_CALLBACK(+[](GtkWidget *, GdkEvent *event, gpointer user_data) {
        auto* navigateur = static_cast<Browser*>(user_data);

        if (event->type == GDK_KEY_PRESS) {
            GdkEventKey* key_event = (GdkEventKey*) event;
            
            if ((key_event->state & GDK_CONTROL_MASK) && key_event->keyval == GDK_KEY_t) {
                navigateur->addNewTab(navigateur->getHomepage());
            } else if ((key_event->state & GDK_CONTROL_MASK) && key_event->keyval == GDK_KEY_w) {
                if (!navigateur->tabs.empty()) {
                    navigateur->removeTab(navigateur->tabs.back().tabWidget);
                }
            } else if ((key_event->state & GDK_CONTROL_MASK) && key_event->keyval == GDK_KEY_d) {
                std::string url = navigateur->getCurrentURL();
                if (!url.empty()) {
                    navigateur->favoritesManager->addFavorite("Favori", url, "");
                    navigateur->refreshFavoritesBar();
                }
            } else if ((key_event->state & GDK_CONTROL_MASK) && key_event->keyval == GDK_KEY_e) {
                // Focus sur la barre d'URL
                if (navigateur->urlBar) {
                    gtk_widget_grab_focus(navigateur->urlBar);
                    gtk_editable_set_position(GTK_EDITABLE(navigateur->urlBar), -1); // Place le curseur a la fin
                }
            }
        }
        return FALSE;
    }), this);
}


void Browser::loadStyles() {
    GtkCssProvider *provider = gtk_css_provider_new();
    GError *error = NULL;

    // CSS amélioré avec design moderne en mode sombre
    const gchar* css = 
        "/* Style global pour forcer le mode sombre */ "
        "window, window * { "
        "  background-color: #2d2d2d; "
        "  color: #e0e0e0; "
        "} "
        "entry { "
        "  background-color: #3d3d3d; "
        "  color: #e0e0e0; "
        "} "
        "textview { "
        "  background-color: #3d3d3d; "
        "  color: #e0e0e0; "
        "} "
        "button { "
        "  background-color: #404040; "
        "  color: #e0e0e0; "
        "} "
        "button:hover { "
        "  background-color: #505050; "
        "} "
        "/* Barre de favorites - Mode sombre avec meilleure visibilité */ "
        "#barre-favorites { "
        "  background-color: #3a3a3a; "
        "  border-bottom: 3px solid rgba(255, 255, 255, 0.3); "
        "  border-top: 1px solid rgba(255, 255, 255, 0.15); "
        "  padding: 6px 10px; "
        "  min-height: 40px; "
        "} "
        "#button-favori { "
        "  border: 1px solid rgba(100, 100, 100, 0.5); "
        "  border-radius: 4px; "
        "  padding: 6px 12px; "
        "  background-color: #4a4a4a; "
        "  color: #e0e0e0; "
        "  margin: 0 3px; "
        "  font-weight: 500; "
        "} "
        "#button-favori:hover { "
        "  background-color: rgba(70, 70, 70, 1.0); "
        "} "
        "#button-favori:active { "
        "  background-color: rgba(90, 90, 90, 1.0); "
        "} "
        "/* Barre de navigation - Mode sombre */ "
        "#barre-navigation { "
        "  padding: 6px 8px; "
        "  border-bottom: 2px solid rgba(255, 255, 255, 0.2); "
        "  background-color: rgba(40, 40, 40, 0.95); "
        "} "
        "/* Barre d'tabs - container visible avec fond sombre */ "
        "#barre-tabs { "
        "  padding: 4px 6px; "
        "  border-bottom: 1px solid rgba(255, 255, 255, 0.12); "
        "  background-color: rgba(35, 35, 35, 0.95); "
        "  border-radius: 4px 4px 0 0; "
        "} "
        "/* Boutons dans la barre d'tabs - Mode sombre */ "
        "#button-groupes, "
        "#button-ajouter-onglet { "
        "  border: 1px solid rgba(100, 100, 100, 0.6); "
        "  border-radius: 4px; "
        "  padding: 4px 8px; "
        "  background-color: rgba(50, 50, 50, 0.9); "
        "  color: rgba(220, 220, 220, 1.0); "
        "  min-width: 28px; "
        "  min-height: 24px; "
        "  font-size: 16px; "
        "  font-weight: bold; "
        "} "
        "#button-ajouter-onglet { "
        "  background-color: rgba(74, 144, 226, 0.9); "
        "  color: white; "
        "  border-color: rgba(74, 144, 226, 1.0); "
        "} "
        "#button-groupes:hover, "
        "#button-ajouter-onglet:hover { "
        "  background-color: rgba(70, 70, 70, 1.0); "
        "  border-color: rgba(120, 120, 120, 0.8); "
        "} "
        "#button-ajouter-onglet:hover { "
        "  background-color: rgba(74, 144, 226, 1.0); "
        "  border-color: rgba(50, 120, 200, 1.0); "
        "} "
        "#button-groupes:active, "
        "#button-ajouter-onglet:active { "
        "  background-color: rgba(90, 90, 90, 1.0); "
        "} "
        "#button-ajouter-onglet:active { "
        "  background-color: rgba(50, 120, 200, 1.0); "
        "} "
        "/* ScrolledWindow pour les onglets - Mode sombre */ "
        "#scrolled-tabs { "
        "  border-bottom: 1px solid rgba(255, 255, 255, 0.12); "
        "  background-color: rgba(35, 35, 35, 0.95); "
        "} "
        "/* Onglets - style amélioré avec séparation claire en mode sombre */ "
        "#onglet { "
        "  border: 2px solid rgba(100, 100, 100, 0.5); "
        "  border-radius: 6px 6px 0 0; "
        "  padding: 6px 12px; "
        "  margin: 0 2px; "
        "  background-color: rgba(50, 50, 50, 0.9); "
        "  border-bottom: none; "
        "  min-height: 32px; "
        "  box-shadow: 0 1px 3px rgba(0, 0, 0, 0.3); "
        "} "
        "#onglet:hover { "
        "  background-color: rgba(70, 70, 70, 1.0); "
        "  border-color: rgba(120, 120, 120, 0.7); "
        "} "
        "#onglet:active { "
        "  background-color: rgba(80, 80, 80, 1.0); "
        "} "
        "/* Onglet actif - style distinctif avec couleur différente en mode sombre */ "
        "#onglet.onglet-actif { "
        "  background-color: rgba(74, 144, 226, 0.9); "
        "  border-color: rgba(90, 160, 240, 1.0); "
        "  border-bottom: 3px solid rgba(90, 160, 240, 1.0); "
        "  font-weight: 600; "
        "  box-shadow: 0 2px 4px rgba(0, 0, 0, 0.4); "
        "} "
        "#onglet.onglet-actif:hover { "
        "  background-color: rgba(90, 160, 240, 1.0); "
        "  border-color: rgba(110, 180, 255, 1.0); "
        "} "
        "/* Labels dans les tabs - Mode sombre */ "
        "#onglet label { "
        "  color: rgba(220, 220, 220, 0.95); "
        "  font-size: 12px; "
        "} "
        "#onglet.onglet-actif label { "
        "  color: rgba(255, 255, 255, 1.0); "
        "  font-weight: 600; "
        "} "
        "/* Bouton fermer dans l'onglet - Mode sombre */ "
        "#onglet button { "
        "  border: 1px solid rgba(100, 100, 100, 0.4); "
        "  background-color: rgba(60, 60, 60, 0.8); "
        "  padding: 2px 6px; "
        "  margin: 0 4px 0 8px; "
        "  border-radius: 4px; "
        "  min-width: 20px; "
        "  min-height: 20px; "
        "  color: rgba(200, 200, 200, 1.0); "
        "  font-weight: bold; "
        "  font-size: 14px; "
        "} "
        "#onglet button:hover { "
        "  background-color: rgba(100, 100, 100, 0.9); "
        "  border-color: rgba(130, 130, 130, 0.6); "
        "  color: rgba(255, 255, 255, 1.0); "
        "} "
        "#onglet button:active { "
        "  background-color: rgba(120, 120, 120, 1.0); "
        "} "
        "#onglet.onglet-actif button { "
        "  background-color: rgba(255, 255, 255, 0.3); "
        "  border-color: rgba(255, 255, 255, 0.5); "
        "  color: rgba(255, 255, 255, 1.0); "
        "} "
        "#onglet.onglet-actif button:hover { "
        "  background-color: rgba(255, 255, 255, 0.5); "
        "  border-color: rgba(255, 255, 255, 0.7); "
        "  color: rgba(255, 255, 255, 1.0); "
        "} "
        "/* Container web et indicateur de chargement - Mode sombre avec debug visuel */ "
        "#container-web { "
        "  background-color: #1e1e1e; "
        "  border-top: 4px solid rgba(255, 255, 255, 0.3); "
        "  min-height: 600px; "
        "} "
        "/* WebView - Fond blanc pour voir le contenu */ "
        "#container-web > * { "
        "  background-color: #ffffff; "
        "} "
        "#loading-indicator { "
        "  background-color: rgba(30, 30, 30, 0.95); "
        "  padding: 20px; "
        "} "
        "#loading-indicator label { "
        "  font-size: 14px; "
        "  color: #cccccc; "
        "} "
        "/* Barre d'URL - Mode sombre */ "
        "#barre-navigation entry { "
        "  background-color: #3d3d3d; "
        "  color: #e0e0e0; "
        "} "
        "/* Tous les widgets de la fenêtre - Mode sombre */ "
        "box, scrolledwindow, viewport { "
        "  background-color: #2d2d2d; "
        "  color: #e0e0e0; "
        "}";

    gtk_css_provider_load_from_data(provider, css, -1, &error);
    
    if (error) {
        g_warning("Erreur de chargement du CSS : %s", error->message);
        g_error_free(error);
        g_object_unref(provider);
        return;
    }

    // Appliquer le style à l'écran avec la priorité la plus élevée
    GtkStyleContext *context = gtk_widget_get_style_context(window);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    
    // Appliquer aussi au screen pour que tous les widgets héritent du style
    GdkScreen *screen = gtk_widget_get_screen(window);
    if (screen) {
        gtk_style_context_add_provider_for_screen(screen, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }
    
    // Ne pas libérer le provider ici - il sera libéré automatiquement par GTK


}


void Browser::onNavigateBack(GtkButton *, Browser *n) {
    if (n->activeTab && n->activeTab->webView) {
        if (webkit_web_view_can_go_back(n->activeTab->webView)) {
            webkit_web_view_go_back(n->activeTab->webView);
        }
    }
}

void Browser::onNavigateForward(GtkButton *, Browser *n) {
    if (n->activeTab && n->activeTab->webView) {
        if (webkit_web_view_can_go_forward(n->activeTab->webView)) {
            webkit_web_view_go_forward(n->activeTab->webView);
        }
    }
}

void Browser::onRefreshPage(GtkButton *, Browser *n) {
    if (n->activeTab && n->activeTab->webView) {
        webkit_web_view_reload(n->activeTab->webView);
    }
}

void Browser::onGoHome(GtkButton *, Browser *n) {
    n->loadURL(n->homepage);
}



void Browser::onUrlBarActivate(GtkEntry* entry, gpointer user_data) {
    auto* navigateur = static_cast<Browser*>(user_data);
    std::string url = gtk_entry_get_text(entry);
    navigateur->loadURL(url);
}

void Browser::closeApplication() {
    saveConfiguration();
    if (window && GTK_IS_WIDGET(window)) {
        // Vérifier que le widget n'est pas déjà en cours de destruction
        if (!gtk_widget_in_destruction(window)) {
            gtk_widget_destroy(window);
        }
        window = nullptr;
    }
    gtk_main_quit();
}

void Browser::onNavigateBackWrapper(GtkButton* button, gpointer user_data) {
    auto* navigateur = static_cast<Browser*>(user_data);
    navigateur->onNavigateBack(button, navigateur);
}



void Browser::onNavigateForwardWrapper(GtkButton *button, gpointer user_data) {
    static_cast<Browser*>(user_data)->onNavigateForward(button, static_cast<Browser*>(user_data));
}

void Browser::onRefreshPageWrapper(GtkButton *button, gpointer user_data) {
    static_cast<Browser*>(user_data)->onRefreshPage(button, static_cast<Browser*>(user_data));
}
void Browser::createContextMenu(GtkWidget* button) {
    GtkWidget *menu = gtk_menu_new();

    // Option "Ajouter aux Favoris"
    GtkWidget *ajouterItem = gtk_menu_item_new_with_label("Ajouter aux Favori");
    g_signal_connect(ajouterItem, "activate", G_CALLBACK(on_ajouter_favori_menu), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), ajouterItem);
    
    // Option "Ouvrir un nouvel onglet"
    GtkWidget *nouvelOngletItem = gtk_menu_item_new_with_label("Ouvrir un nouvel onglet");
    g_signal_connect(nouvelOngletItem, "activate", G_CALLBACK(on_ajouter_onglet), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), nouvelOngletItem);

    auto* data = new std::pair<Browser*, GtkWidget*>(this, button);
    // Option "Supprimer le favori" (Ajout si applicable)
    GtkWidget *supprimerItem = gtk_menu_item_new_with_label("Supprimer le favori");
    g_signal_connect_data(supprimerItem, "activate", G_CALLBACK(on_supprimer_favori),
                      data, delete_user_data, G_CONNECT_AFTER);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), supprimerItem);

    gtk_widget_show_all(menu);  // Ajout avant l'ouverture du menu
    gtk_menu_popup_at_widget(GTK_MENU(menu), button, GDK_GRAVITY_SOUTH, GDK_GRAVITY_NORTH, nullptr);
}


void Browser::onStarButtonClicked(GtkButton* button, gpointer user_data) {
    auto* navigateur = static_cast<Browser*>(user_data);
    if (!navigateur) return;

    // Obtenir l'URL actuelle et pré-remplir l'entrée
    std::string urlActuelle = navigateur->getCurrentURL();
    gtk_entry_set_text(GTK_ENTRY(navigateur->favoriteUrlEntry), urlActuelle.c_str());

    // Vérifier si l'URL est déjà dans les favorites (y compris dans un dossier)
    auto favorites = navigateur->getFavoris();
    const bool estDejaFavori = FavoritesJson::containsUrlRecursive(*favorites, urlActuelle);

    // Si l'URL est déjà un favori, afficher un message et ne pas ouvrir le popover
    if (estDejaFavori) {
        std::cerr << "URL déjà ajoutée aux favorites : " << urlActuelle << std::endl;
        gtk_button_set_label(button, "*"); // Marquer l'URL comme déjà en favorites
        navigateur->showFavoritesManager(); // Permet la modification
    } else {
        // Sinon, ouvrir le popover pour permettre l'ajout
        gtk_button_set_label(button, "☆");  // Étoile vide
        gtk_popover_popup(GTK_POPOVER(navigateur->getPopoverFavoris()));
    }
}

void Browser::showCommandPalette() {
    if (commandPalette && window) {
        commandPalette->setCurrentWebView(renderingEngine.get());
        commandPalette->showPalette(GTK_WINDOW(window));
    }
}

void Browser::toggleCommandPalette() {
    if (!commandPalette || !window) return;
    
    // Vérifier si la palette est déjà visible
    if (commandPalette->isVisible()) {
        // Si elle est visible, la masquer
        commandPalette->hidePalette();
    } else {
        // Si elle n'est pas visible, l'afficher
        commandPalette->setCurrentWebView(renderingEngine.get());
        commandPalette->showPalette(GTK_WINDOW(window));
    }
}

// Fonctions hibernerOnglet et reactiverOnglet supprimées - gérées par MemoryManager
