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
#include <gdk-pixbuf/gdk-pixbuf.h>

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


static void onStarButtonClicked(GtkButton* button, gpointer user_data) {
    auto* navigateur = static_cast<Browser*>(user_data);
    if (navigateur) {
        navigateur->addFavorite("Favori", navigateur->getCurrentURL(), "");
    }
}

static void on_menu_item_activate(GtkWidget* widget, gpointer user_data) {
    auto* data = static_cast<std::pair<Browser*, std::string>*>(user_data);
    data->first->loadURL(data->second);
    delete data;
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

static void on_favori_clicked(GtkButton* button, gpointer user_data) {
    auto* data = static_cast<std::pair<Browser*, std::string>*>(user_data);
    if (data && data->first) {
        data->first->loadURL(data->second);
        delete data;  // Libérer la mémoire allouée
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
        
        // Gestion du raccourci CTRL + ALT + C pour la palette de commandes
        if ((key_event->state & (GDK_CONTROL_MASK | GDK_MOD1_MASK)) && 
            key_event->keyval == GDK_KEY_c) {
            navigateur->showCommandPalette();
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
      favoriteNameEntry(nullptr),
      favoriteUrlEntry(nullptr),
      favoritesPopover(nullptr)
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
    favoriteNameEntry = nullptr;
    favoriteUrlEntry = nullptr;
    favoritesPopover = nullptr;
}

std::shared_ptr<nlohmann::json> Browser::getFavoris() {
    return favorites;
}

void Browser::buildInterface() {
    std::cerr << "[DEBUG] Début de buildInterface()" << std::endl;
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    std::cerr << "[DEBUG] Fenêtre créée" << std::endl;
    gtk_window_set_title(GTK_WINDOW(window), "WeedlyWeb");
    gtk_window_set_default_size(GTK_WINDOW(window), 1024, 768);
    
    // Définir l'icône de la fenêtre
    std::string iconPath = FileManager::obtenirCheminAbsolu("assets/icons/weedlyweb.png");
    if (std::filesystem::exists(iconPath)) {
        GdkPixbuf *icon = gdk_pixbuf_new_from_file(iconPath.c_str(), nullptr);
        if (icon) {
            gtk_window_set_icon(GTK_WINDOW(window), icon);
            g_object_unref(icon);
            std::cerr << "[DEBUG] Icône chargée : " << iconPath << std::endl;
        }
    } else {
        // Essayer avec le SVG si PNG n'existe pas
        std::string svgPath = FileManager::obtenirCheminAbsolu("assets/icons/weedlyweb.svg");
        if (std::filesystem::exists(svgPath)) {
            GdkPixbuf *icon = gdk_pixbuf_new_from_file(svgPath.c_str(), nullptr);
            if (icon) {
                gtk_window_set_icon(GTK_WINDOW(window), icon);
                g_object_unref(icon);
                std::cerr << "[DEBUG] Icône SVG chargée : " << svgPath << std::endl;
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
    
    std::cerr << "[DEBUG] Fenêtre configurée" << std::endl;

    // Connexion sécurisée du signal de fermeture avec lambda sécurisée
    g_signal_connect(window, "delete-event", G_CALLBACK(+[](GtkWidget*, GdkEvent*, gpointer user_data) -> gboolean {
        auto* navigateur = static_cast<Browser*>(user_data);
        navigateur->saveConfiguration();
        gtk_main_quit();  // Quitter proprement l'application
        return FALSE;  
    }), this);

    // Conteneur principal
    std::cerr << "[DEBUG] Création du container principal" << std::endl;
    mainContainer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window), mainContainer);
    std::cerr << "[DEBUG] Conteneur principal ajouté à la fenêtre" << std::endl;

    // CORRECT ORDER: Bars at top, web view at bottom (expandable)
    // 1. Tabs bar (at top)
    std::cerr << "[DEBUG] Initialisation de la barre d'tabs..." << std::endl;
    initializeTabsBar();
    std::cerr << "[DEBUG] Barre d'tabs initialisée" << std::endl;
    
    // 2. Navigation bar with URL (under tabs)
    std::cerr << "[DEBUG] Initialisation de la barre de navigation..." << std::endl;
    initializeNavigationBar();
    std::cerr << "[DEBUG] Barre de navigation initialisée" << std::endl;
    
    // 3. Favorites bar (under URL bar)
    std::cerr << "[DEBUG] Initialisation de la barre de favorites..." << std::endl;
    // Ne pas appeler initializeFavoritesBar() ici car refreshFavoritesBar() le fait déjà
    refreshFavoritesBar();
    std::cerr << "[DEBUG] Barre de favorites initialisée" << std::endl;
    
    // 4. Web rendering zone (at bottom, expandable) - MUST be last to take remaining space
    std::cerr << "[DEBUG] Initialisation du moteur de rendu..." << std::endl;
    renderingEngine->initializeRendering(mainContainer);
    std::cerr << "[DEBUG] Moteur de rendu initialisé" << std::endl;
    
    // Afficher la fenêtre
    std::cerr << "[DEBUG] Affichage de la fenêtre..." << std::endl;
    gtk_widget_show_all(window);
    
    // Présenter la fenêtre au gestionnaire de fenêtres (pour qu'elle apparaisse dans la barre des tâches)
    gtk_window_present(GTK_WINDOW(window));
    
    std::cerr << "[DEBUG] Fenêtre affichée" << std::endl;
    
    // Ajouter le premier onglet et charger la page d'accueil
    std::cerr << "[DEBUG] Ajout du premier onglet..." << std::endl;
    std::string homepageUrl = homepage.empty() ? "https://www.duckduckgo.com" : homepage;
    addNewTab(homepageUrl);
    std::cerr << "[DEBUG] Premier onglet ajouté avec URL: " << homepageUrl << std::endl;
    
    // Charger les styles CSS pour un design minimaliste
    loadStyles();
    
    std::cerr << "[DEBUG] Fin de buildInterface()" << std::endl;

    renderingEngine->connectURLChangedSignal([this](const std::string& url) {
        if (urlBar) {
            gtk_entry_set_text(GTK_ENTRY(urlBar), url.c_str());
        }
    });

    starButton = renderingEngine->createButton("☆", G_CALLBACK(on_bouton_favoris_clicked), this);
    gtk_box_pack_start(GTK_BOX(navigationBar), starButton, FALSE, FALSE, 0);

    initializeFavoritesPopover();
    // gtk_widget_show_all sera appelé après l'ajout de l'onglet
}

void Browser::addButton(GtkWidget* container, const std::string& iconName, GCallback callback, gpointer data) {
    GtkWidget *button = renderingEngine->createButton(iconName, callback, data);
    if (!gtk_widget_get_parent(button)) {  // Éviter les duplications
        gtk_box_pack_start(GTK_BOX(container), button, FALSE, FALSE, 0);
    }
}

std::string Browser::getCurrentTitle() const {
    return renderingEngine->getCurrentTitle();
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
    
    // Barre d'URL (expandable)
    urlBar = renderingEngine->createTextEntry(G_CALLBACK(&Browser::onUrlBarActivate), this);
    gtk_box_pack_start(GTK_BOX(navigationBar), urlBar, TRUE, TRUE, 0);

    // Bouton favorites (étoile)
    starButton = renderingEngine->createButton("☆", G_CALLBACK(on_bouton_favoris_clicked), this);
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
    return renderingEngine->getCurrentURL();
}


void Browser::initializeFavoritesBar() {
    if (favoritesBar && GTK_IS_WIDGET(favoritesBar)) {
        // Retirer du container avant de détruire
        GtkWidget* parent = gtk_widget_get_parent(favoritesBar);
        if (parent && GTK_IS_CONTAINER(parent)) {
            gtk_container_remove(GTK_CONTAINER(parent), favoritesBar);
        }
        gtk_widget_destroy(favoritesBar);
        favoritesBar = nullptr;
    }

    // Créer une barre de favorites minimaliste et élégante
    favoritesBar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3);
    gtk_widget_set_margin_start(favoritesBar, 5);
    gtk_widget_set_margin_end(favoritesBar, 5);
    gtk_widget_set_margin_top(favoritesBar, 2);
    gtk_widget_set_margin_bottom(favoritesBar, 2);
    
    // Style minimaliste pour la barre de favorites
    gtk_widget_set_name(favoritesBar, "barre-favorites");

    // Afficher les favorites (maximum 10 visibles, le reste dans un menu)
    int maxFavorisVisibles = 10;
    int compteur = 0;

    for (const auto& favori : *favorites) {
        if (compteur >= maxFavorisVisibles) {
            // Bouton "..." pour afficher les favorites restants
            GtkWidget* boutonPlus = gtk_button_new_with_label("⋯");
            gtk_widget_set_tooltip_text(boutonPlus, "Plus de favorites");
            gtk_widget_set_margin_start(boutonPlus, 2);
            gtk_widget_set_margin_end(boutonPlus, 2);
            g_signal_connect(boutonPlus, "clicked", G_CALLBACK(+[](GtkButton*, gpointer user_data) {
                auto* navigateur = static_cast<Browser*>(user_data);
                navigateur->showRemainingFavoritesMenu();
            }), this);
            gtk_box_pack_start(GTK_BOX(favoritesBar), boutonPlus, FALSE, FALSE, 0);
            break;
        }

        // Créer un button de favori avec un style minimaliste
        std::string favoriteName = favori.value("name", "Favori");
        // Limiter la longueur du nom pour un design propre
        if (favoriteName.length() > 15) {
            favoriteName = favoriteName.substr(0, 12) + "...";
        }
        
        GtkWidget* boutonFavori = gtk_button_new_with_label(favoriteName.c_str());
        gtk_widget_set_tooltip_text(boutonFavori, favori.value("url", "").c_str());
        gtk_widget_set_margin_start(boutonFavori, 2);
        gtk_widget_set_margin_end(boutonFavori, 2);
        
        auto* data = new std::pair<Browser*, std::string>(this, favori["url"]);
        g_signal_connect(boutonFavori, "clicked", G_CALLBACK(on_favori_clicked), data);
        
        // Style minimaliste pour les boutons de favorites
        gtk_widget_set_name(boutonFavori, "button-favori");
        
        gtk_box_pack_start(GTK_BOX(favoritesBar), boutonFavori, FALSE, FALSE, 0);
        compteur++;
    }

    // Si aucun favori, afficher un message discret
    if (favorites->empty()) {
        GtkWidget* labelVide = gtk_label_new("");
        gtk_widget_set_opacity(labelVide, 0.0); // Invisible mais prend de l'espace
        gtk_box_pack_start(GTK_BOX(favoritesBar), labelVide, FALSE, FALSE, 0);
    }

    // Ajouter la barre de favorites au container principal
    if (mainContainer && !gtk_widget_get_parent(favoritesBar)) {
        gtk_box_pack_start(GTK_BOX(mainContainer), favoritesBar, FALSE, FALSE, 0);
    }
    gtk_widget_show_all(favoritesBar);
}

void Browser::showFavoritesMenu() {
    GtkWidget* menu = gtk_menu_new();

    int largeurDispo = gtk_widget_get_allocated_width(mainContainer);
    int largeurActuelle = 0;


    for (const auto& favori : *favorites) {
        int largeurBouton = 80; // Estimation
        if (largeurActuelle + largeurBouton > largeurDispo) {
            GtkWidget* item = gtk_menu_item_new_with_label(favori["name"].get<std::string>().c_str());
            auto* data = new std::pair<Browser*, std::string>(this, favori["url"]);
            g_signal_connect(item, "activate", G_CALLBACK(on_menu_item_activate), data);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        }
        largeurActuelle += largeurBouton;
    }

    gtk_widget_show_all(menu);
    gtk_menu_popup_at_widget(GTK_MENU(menu), favoritesBar, GDK_GRAVITY_SOUTH, GDK_GRAVITY_NORTH, nullptr);
}

void Browser::showRemainingFavoritesMenu() {
    GtkWidget* menu = gtk_menu_new();
    int largeurDispo = gtk_widget_get_allocated_width(mainContainer);
    int largeurActuelle = 0;

    for (const auto& favori : *favorites) {
        int largeurBouton = 80; // Estimation de la largeur d'un button
        if (largeurActuelle + largeurBouton > largeurDispo) {
            GtkWidget* item = gtk_menu_item_new_with_label(favori["name"].get<std::string>().c_str());
            auto* data = new std::pair<Browser*, std::string>(this, favori["url"]);
            g_signal_connect(item, "activate", G_CALLBACK(on_menu_item_activate), data);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        }
        largeurActuelle += largeurBouton;
    }

    gtk_widget_show_all(menu);
    gtk_menu_popup_at_widget(GTK_MENU(menu), favoritesBar, GDK_GRAVITY_SOUTH, GDK_GRAVITY_NORTH, nullptr);
}


void Browser::showFavoritesManager() {
    if (!favoritesManager) {
        favoritesManager = std::make_unique<FavoritesManager>(favorites, [this]() { refreshFavoritesBar(); });
    }
    favoritesManager->showWindow();
}


void Browser::refreshFavoritesBar() {
    if (favoritesBar && GTK_IS_WIDGET(favoritesBar)) {
        // Vérifier que le widget n'est pas déjà en cours de destruction
        if (!gtk_widget_in_destruction(favoritesBar)) {
            // Retirer du container avant de détruire
            GtkWidget* parent = gtk_widget_get_parent(favoritesBar);
            if (parent && GTK_IS_CONTAINER(parent)) {
                gtk_container_remove(GTK_CONTAINER(parent), favoritesBar);
            }
            gtk_widget_destroy(favoritesBar);
        }
        favoritesBar = nullptr;
    }

    favoritesBar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

    if (favorites->empty()) {
        GtkWidget *labelAucunFavori = gtk_label_new("Aucun favori");
        gtk_box_pack_start(GTK_BOX(favoritesBar), labelAucunFavori, FALSE, FALSE, 5);
    }

    for (const auto& favori : *favorites) {
        GtkWidget *boutonFavori = renderingEngine->createButton(favori["name"], nullptr, nullptr);
        auto* data = new std::pair<Browser*, std::string>(this, favori["url"]);
        g_signal_connect(boutonFavori, "clicked", G_CALLBACK(on_favori_clicked), data);
        if (gtk_widget_get_parent(boutonFavori) == nullptr) {
            gtk_box_pack_start(GTK_BOX(favoritesBar), boutonFavori, FALSE, FALSE, 5);
        }
    }

    // Vérifier que favoritesBar n'est pas déjà dans le container
    // IMPORTANT: Ajouter après la barre de navigation mais avant la zone web
    if (mainContainer && !gtk_widget_get_parent(favoritesBar)) {
        // Ajouter après la barre de navigation
        gtk_box_pack_start(GTK_BOX(mainContainer), favoritesBar, FALSE, FALSE, 0);
        // Réordonner pour s'assurer que c'est après la barre de navigation (position 2)
        gtk_box_reorder_child(GTK_BOX(mainContainer), favoritesBar, 2);
    }
    gtk_widget_show_all(favoritesBar);
}


void Browser::updateStarButton() {
    std::string urlActuelle = getCurrentURL();
    bool estDejaFavori = std::any_of(
        favorites->begin(), favorites->end(),
        [&urlActuelle](const nlohmann::json& favori) { return favori["url"] == urlActuelle; }
    );


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
    if (tabsBar && GTK_IS_WIDGET(tabsBar)) {
        GtkWidget* parent = gtk_widget_get_parent(tabsBar);
        if (parent && GTK_IS_CONTAINER(parent)) {
            gtk_container_remove(GTK_CONTAINER(parent), tabsBar);
        }
        gtk_widget_destroy(tabsBar);
        tabsBar = nullptr;
    }

    // Créer une nouvelle barre d'tabs avec container visible
    tabsBar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_widget_set_margin_start(tabsBar, 5);
    gtk_widget_set_margin_end(tabsBar, 5);
    gtk_widget_set_margin_top(tabsBar, 5);
    gtk_widget_set_margin_bottom(tabsBar, 2);
    gtk_widget_set_name(tabsBar, "barre-tabs");
    
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
    
    // Les tabs seront ajoutés ici (au milieu)
    
    // Bouton "+" à droite pour ajouter un nouvel onglet
    GtkWidget* boutonAjouterOnglet = gtk_button_new_with_label("+");
    gtk_widget_set_tooltip_text(boutonAjouterOnglet, "Ajouter un nouvel onglet");
    gtk_widget_set_margin_start(boutonAjouterOnglet, 2);
    gtk_widget_set_margin_end(boutonAjouterOnglet, 2);
    gtk_widget_set_name(boutonAjouterOnglet, "button-ajouter-onglet");
    g_signal_connect(boutonAjouterOnglet, "clicked", G_CALLBACK(on_ajouter_onglet), this);
    gtk_box_pack_end(GTK_BOX(tabsBar), boutonAjouterOnglet, FALSE, FALSE, 0);
    
    // Ajouter la barre d'tabs au container principal (en haut)
    if (mainContainer && !gtk_widget_get_parent(tabsBar)) {
        gtk_box_pack_start(GTK_BOX(mainContainer), tabsBar, FALSE, FALSE, 0);
    }
}

void Browser::changeTabGroup(const std::string& groupName) {
    tabsManager->changerGroupeActif(groupName);
    // Retirer du container avant de détruire
    if (tabsBar && GTK_IS_WIDGET(tabsBar)) {
        GtkWidget* parent = gtk_widget_get_parent(tabsBar);
        if (parent && GTK_IS_CONTAINER(parent)) {
            gtk_container_remove(GTK_CONTAINER(parent), tabsBar);
        }
        gtk_widget_destroy(tabsBar);
        tabsBar = nullptr;
    }
    initializeTabsBar();
}


void Browser::addNewTab(const std::string &url) {
    std::cerr << "[DEBUG] Début de addNewTab(" << url << ")" << std::endl;
    tabsManager->ajouterOnglet(tabsManager->getGroupeActif(), url);
    std::cerr << "[DEBUG] Onglet ajouté au gestionnaire" << std::endl;

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
    g_signal_connect(boutonFermer, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer user_data) {
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
    g_signal_connect(hboxOnglet, "button-press-event", G_CALLBACK(+[](GtkWidget* widget, GdkEventButton*, gpointer user_data) {
        auto* n = static_cast<Browser*>(user_data);
        if (n) {
            n->changeActiveTab(widget);
        }
        return TRUE;
    }), this);

    // Ajouter l'onglet à la barre d'tabs (après le button groupes, avant le button +)
    if (tabsBar && !gtk_widget_get_parent(hboxOnglet)) {
        // Insérer après le premier enfant (le button groupes)
        GList* children = gtk_container_get_children(GTK_CONTAINER(tabsBar));
        if (children) {
            // Insérer après le premier enfant (button groupes)
            gtk_box_pack_start(GTK_BOX(tabsBar), hboxOnglet, FALSE, FALSE, 0);
            g_list_free(children);
        } else {
            gtk_box_pack_start(GTK_BOX(tabsBar), hboxOnglet, FALSE, FALSE, 0);
        }
    }
    tabs.push_back({url, hboxOnglet});
    
    // Afficher l'onglet
    gtk_widget_show_all(hboxOnglet);
    
    // Changer l'onglet actif vers celui-ci
    changeActiveTab(hboxOnglet);
    
    // Afficher la page - IMPORTANT: s'assurer que la webView est visible
    std::cerr << "[DEBUG] Affichage de la page: " << url << std::endl;
    if (renderingEngine && !url.empty()) {
        // Normaliser l'URL si nécessaire
        std::string urlNormalisee = url;
        if (url.find("://") == std::string::npos) {
            urlNormalisee = "https://" + url;
        }
        renderingEngine->displayPage(urlNormalisee);
        
        // S'assurer que la webView est visible et expandable
        WebKitWebView* webView = renderingEngine->getVueWeb();
        if (webView) {
            GtkWidget* widget = GTK_WIDGET(webView);
            gtk_widget_set_vexpand(widget, TRUE);
            gtk_widget_set_hexpand(widget, TRUE);
            gtk_widget_show(widget);
            // S'assurer que le parent est aussi visible
            GtkWidget* parent = gtk_widget_get_parent(widget);
            if (parent) {
                gtk_widget_show_all(parent);
            }
        }
        
        std::cerr << "[DEBUG] Page affichée" << std::endl;
    }
    
    // Connecter le signal pour mettre à jour le titre de l'onglet quand la page se charge
    if (renderingEngine) {
        renderingEngine->connectPageLoadedSignal([labelTitre](const std::string &titre) {
            if (labelTitre && GTK_IS_LABEL(labelTitre)) {
                std::string titreCourt = titre.length() > 20 ? titre.substr(0, 17) + "..." : titre;
                gtk_label_set_text(GTK_LABEL(labelTitre), titreCourt.c_str());
            }
        });
    }
    
    std::cerr << "[DEBUG] Fin de addNewTab()" << std::endl;


    gtk_widget_show_all(tabsBar);
}

void Browser::changeActiveTab(GtkWidget* tabWidget) {
    for (auto &[url, widget] : tabs) {
        if (widget == tabWidget) {
            renderingEngine->displayPage(url);
            highlight(widget);
            return;
        }
    }
}



void Browser::executeScriptInActiveTab(const std::string& script) {
    if (!tabs.empty()) {
        GtkWidget* ongletActif = tabs.back().second;
        WebKitWebView* webView = nullptr;
        GList* children = gtk_container_get_children(GTK_CONTAINER(ongletActif));
        if (children != nullptr) {
            webView = WEBKIT_WEB_VIEW(children->data);
            g_list_free(children);
        }        
        scriptEngine->executerScript(webView, script);
    }
}

void Browser::removeFavorite(GtkWidget* widget) {
    const gchar* favoriteName = gtk_button_get_label(GTK_BUTTON(widget));
    favoritesManager->removeFavorite(favoriteName);
    refreshFavoritesBar();  // Mise à jour visuelle
}


void Browser::removeTab(GtkWidget *tabWidget) {
    // Trouver l'onglet à supprimer
    auto it = std::find_if(tabs.begin(), tabs.end(), [tabWidget](const auto &pair) {
        return pair.second == tabWidget;
    });

    if (it != tabs.end()) {
        GtkWidget *hboxOnglet = it->second;

        WebKitWebView* webView = nullptr;  // Déclaration ici

        GList* children = gtk_container_get_children(GTK_CONTAINER(hboxOnglet));
        if (children != nullptr) {
            for (GList* iter = children; iter != nullptr; iter = iter->next) {
                if (WEBKIT_IS_WEB_VIEW(iter->data)) {
                    webView = WEBKIT_WEB_VIEW(iter->data);
                    break;
                }
            }
            g_list_free(children);
        }

        if (webView && WEBKIT_IS_WEB_VIEW(webView)) {
            memoryManager->hibernerOnglet(webView); // Mise en veille de l'onglet
        }

        // Supprimer l'onglet de la liste
        if (hboxOnglet && GTK_IS_WIDGET(hboxOnglet)) {
            // Vérifier que le widget n'est pas déjà en cours de destruction
            if (!gtk_widget_in_destruction(hboxOnglet)) {
                // Retirer du container avant de détruire
                GtkWidget* parent = gtk_widget_get_parent(hboxOnglet);
                if (parent && GTK_IS_CONTAINER(parent)) {
                    gtk_container_remove(GTK_CONTAINER(parent), hboxOnglet);
                }
                gtk_widget_destroy(hboxOnglet);
            }
        }
        tabs.erase(it);

        // Si aucun onglet n'est présent, fermer l'application
        if (tabs.empty()) {
            closeApplication();
        } else {
            // Charger l'URL du premier onglet
            loadURL(tabs.front().first);
        }
    }
}

void Browser::highlight(GtkWidget *tabWidget) {
    if (!tabWidget) return;
    
    // Parcourir tous les tabs et gérer les classes CSS
    for (auto &[url, widget] : tabs) {
        if (widget && GTK_IS_WIDGET(widget)) {
            GtkStyleContext* context = gtk_widget_get_style_context(widget);
            if (widget == tabWidget) {
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
    renderingEngine->displayPage(url);
    history.push_back(url);

    if (urlBar) {
        gtk_entry_set_text(GTK_ENTRY(urlBar), url.c_str());
    }

    // Mettre à jour le titre de l'onglet actif
    renderingEngine->connectTitleChangedSignal([this](const std::string& titre) {
        if (!tabs.empty()) {
            GtkWidget* hboxOnglet = tabs.back().second;
            GList* children = gtk_container_get_children(GTK_CONTAINER(hboxOnglet));
            if (children) {
                GtkWidget* labelTitre = GTK_WIDGET(children->data);
                if (GTK_IS_LABEL(labelTitre)) {
                    gtk_label_set_text(GTK_LABEL(labelTitre), titre.c_str());
                }
                g_list_free(children);
            }
        }
    });
    memoryManager->optimiserMemoire();
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
        config["homepage"] = "https://www.duckduckgo.com";
        FileManager::writeJSON(chemin, config);
    }

    if (config.contains("tabs")) {
        for (const auto& onglet : config["tabs"]) {
            if (onglet.contains("url")) {
                addNewTab(onglet["url"]);
            }
        }
    }
    
    homepage = config.value("homepage", "https://www.duckduckgo.com");
    std::cout << "Page d'accueil définie sur : " << homepage << std::endl;
}


void Browser::saveConfiguration() {
    nlohmann::json config;
    config["homepage"] = homepage;
    FileManager::writeJSON(FileManager::configJSONPath(), config);
    
    FileManager::writeJSON(FileManager::favoritesJSONPath(), *favorites);
    std::cout << "Favoris sauvegardés automatiquement !" << std::endl;
    
    nlohmann::json ongletsJson = nlohmann::json::array();
    for (const auto& [url, widget] : tabs) {
        ongletsJson.push_back({{"url", url}});
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
    g_signal_connect(itemAide, "activate", G_CALLBACK(+[](GtkWidget*, gpointer) {
        GtkWidget* dialog = gtk_message_dialog_new(
            nullptr,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_INFO,
            GTK_BUTTONS_OK,
            "Raccourcis clavier:\n\n"
            "Ctrl+T : Nouvel onglet\n"
            "Ctrl+W : Fermer l'onglet\n"
            "Ctrl+D : Ajouter aux favorites\n"
            "Ctrl+E : Focus barre URL"
        );
        gtk_dialog_run(GTK_DIALOG(dialog));
        if (dialog && GTK_IS_WIDGET(dialog)) {
            gtk_widget_destroy(dialog);
        }
    }), nullptr);
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
        if (dialog && GTK_IS_WIDGET(dialog)) {
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
        gtk_widget_destroy(dialog);
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
    
    // Trouver le button groupes pour positionner le popup
    GList* children = gtk_container_get_children(GTK_CONTAINER(tabsBar));
    GtkWidget* boutonGroupes = nullptr;
    if (children) {
        GList* last = g_list_last(children);
        if (last && GTK_IS_BUTTON(GTK_WIDGET(last->data))) {
            boutonGroupes = GTK_WIDGET(last->data);
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
    std::cerr << "[DEBUG] Chargement des paramètres depuis: " << urlParametres << std::endl;
    
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
        if (dialog && GTK_IS_WIDGET(dialog)) {
            gtk_widget_destroy(dialog);
        }
        return;
    }
    fichier.close();
    
    renderingEngine->displayPage(urlParametres);
    
    // S'assurer que la webView est visible
    WebKitWebView* webView = renderingEngine->getVueWeb();
    if (webView) {
        gtk_widget_show_all(GTK_WIDGET(webView));
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
                    navigateur->removeTab(navigateur->tabs.back().second);
                }
            } else if ((key_event->state & GDK_CONTROL_MASK) && key_event->keyval == GDK_KEY_d) {
                std::string url = navigateur->renderingEngine->getCurrentURL();
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

    // CSS amélioré avec design moderne et container visible
    const gchar* css = 
        "/* Barre de favorites */ "
        "#barre-favorites { "
        "  background-color: rgba(245, 245, 245, 0.95); "
        "  border-bottom: 1px solid rgba(0, 0, 0, 0.1); "
        "  padding: 4px 8px; "
        "} "
        "#button-favori { "
        "  border: none; "
        "  border-radius: 4px; "
        "  padding: 4px 8px; "
        "  background-color: rgba(255, 255, 255, 0.9); "
        "  margin: 0 2px; "
        "} "
        "#button-favori:hover { "
        "  background-color: rgba(230, 230, 230, 1.0); "
        "} "
        "#button-favori:active { "
        "  background-color: rgba(210, 210, 210, 1.0); "
        "} "
        "/* Barre de navigation */ "
        "#barre-navigation { "
        "  padding: 6px 8px; "
        "  border-bottom: 1px solid rgba(0, 0, 0, 0.1); "
        "  background-color: rgba(250, 250, 250, 0.95); "
        "} "
        "/* Barre d'tabs - container visible avec fond */ "
        "#barre-tabs { "
        "  padding: 4px 6px; "
        "  border-bottom: 1px solid rgba(0, 0, 0, 0.12); "
        "  background-color: rgba(235, 235, 235, 0.95); "
        "  border-radius: 4px 4px 0 0; "
        "} "
        "/* Boutons dans la barre d'tabs */ "
        "#button-groupes, "
        "#button-ajouter-onglet { "
        "  border: 1px solid rgba(180, 180, 180, 0.6); "
        "  border-radius: 4px; "
        "  padding: 4px 8px; "
        "  background-color: rgba(255, 255, 255, 0.9); "
        "  min-width: 28px; "
        "  min-height: 24px; "
        "} "
        "#button-groupes:hover, "
        "#button-ajouter-onglet:hover { "
        "  background-color: rgba(240, 240, 240, 1.0); "
        "  border-color: rgba(150, 150, 150, 0.8); "
        "} "
        "#button-groupes:active, "
        "#button-ajouter-onglet:active { "
        "  background-color: rgba(220, 220, 220, 1.0); "
        "} "
        "/* Onglets - style amélioré pour montrer qu'ils sont dans le même container */ "
        "#onglet { "
        "  border: 1px solid rgba(180, 180, 180, 0.5); "
        "  border-radius: 4px 4px 0 0; "
        "  padding: 6px 12px; "
        "  margin: 0 2px; "
        "  background-color: rgba(245, 245, 245, 0.9); "
        "  border-bottom: none; "
        "  min-height: 28px; "
        "} "
        "#onglet:hover { "
        "  background-color: rgba(235, 235, 235, 1.0); "
        "  border-color: rgba(150, 150, 150, 0.7); "
        "} "
        "#onglet:active { "
        "  background-color: rgba(225, 225, 225, 1.0); "
        "} "
        "/* Onglet actif - style distinctif */ "
        "#onglet.onglet-actif { "
        "  background-color: rgba(255, 255, 255, 1.0); "
        "  border-color: rgba(74, 144, 226, 0.8); "
        "  border-bottom: 2px solid #4A90E2; "
        "  border-bottom-width: 2px; "
        "  font-weight: 500; "
        "} "
        "#onglet.onglet-actif:hover { "
        "  background-color: rgba(250, 250, 250, 1.0); "
        "} "
        "/* Labels dans les tabs */ "
        "#onglet label { "
        "  color: rgba(50, 50, 50, 0.9); "
        "} "
        "#onglet.onglet-actif label { "
        "  color: rgba(30, 30, 30, 1.0); "
        "} "
        "/* Bouton fermer dans l'onglet */ "
        "#onglet button { "
        "  border: none; "
        "  background-color: transparent; "
        "  padding: 2px 4px; "
        "  margin: 0 2px; "
        "  border-radius: 3px; "
        "  min-width: 18px; "
        "  min-height: 18px; "
        "} "
        "#onglet button:hover { "
        "  background-color: rgba(220, 220, 220, 0.8); "
        "} "
        "#onglet button:active { "
        "  background-color: rgba(200, 200, 200, 1.0); "
        "}";

    gtk_css_provider_load_from_data(provider, css, -1, &error);
    
    if (error) {
        g_warning("Erreur de chargement du CSS : %s", error->message);
        g_error_free(error);
        g_object_unref(provider);
        return;
    }

    // Appliquer le style à l'écran
    GtkStyleContext *context = gtk_widget_get_style_context(window);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    
    // Ne pas libérer le provider ici - il sera libéré automatiquement par GTK


}


void Browser::onNavigateBack(GtkButton *, Browser *n) {
    n->renderingEngine->navigateBack();
}

void Browser::onNavigateForward(GtkButton *, Browser *n) {
    n->renderingEngine->navigateForward();
}

void Browser::onRefreshPage(GtkButton *, Browser *n) {
    n->loadURL(n->renderingEngine->getCurrentURL());
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

    // Vérifier si l'URL est déjà dans les favorites
    auto favorites = navigateur->getFavoris();
    bool estDejaFavori = std::any_of(
        favorites->begin(),
        favorites->end(),
        [&urlActuelle](const nlohmann::json& favori) { return favori["url"] == urlActuelle; }
    );

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

// Fonctions hibernerOnglet et reactiverOnglet supprimées - gérées par MemoryManager
