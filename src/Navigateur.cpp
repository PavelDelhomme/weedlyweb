#include "Navigateur.h"
#include "GestionnaireFichiers.h"
#include "GestionnaireMemoire.h"
#include "GestionnaireFavoris.h"
#include "Utils.h"
#include "utils/CommandPalette.h"
#include "utils/RequestInterceptor.h"
#include "database/Database.h"
#include <iostream>
#include <filesystem>
#include <string>
#include <fstream>
#include <set>

std::string obtenirCheminAbsolu(const std::string& fichier) {
    return std::filesystem::current_path().string() + "/" + fichier;
}
static void delete_user_data(gpointer user_data, GClosure*) {
    delete static_cast<std::pair<Navigateur*, GtkWidget*>*>(user_data);
}

// Déclarations pour Utils.cpp (non-static pour être accessibles depuis Utils.cpp)
void on_ouvrir_nouvel_onglet_safe(GtkWidget*, gpointer user_data);
void on_modifier_favori(GtkWidget*, gpointer user_data);
void on_supprimer_favori(GtkWidget*, gpointer user_data);

static gboolean on_favori_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    if (event->type == GDK_BUTTON_PRESS && event->button == GDK_BUTTON_SECONDARY) {
        auto* navigateur = static_cast<Navigateur*>(user_data);
        GtkWidget* menu = creerMenuContextuelFavoris(navigateur, widget);
        gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent*)event);
        return TRUE;
    }
    return FALSE;
}



// Définition pour Utils.cpp
void on_ouvrir_nouvel_onglet_safe(GtkWidget*, gpointer user_data) {
    auto* data = static_cast<std::pair<Navigateur*, std::string>*>(user_data);
    if (!data) return;
    if (data->first) {
        data->first->ajouterNouvelOnglet(data->second);
    }
    delete data;
}

// Définition pour Utils.cpp
void on_supprimer_favori(GtkWidget* widget, gpointer user_data) {
    auto* data = static_cast<std::pair<Navigateur*, GtkWidget*>*>(user_data);
    if (data && data->first) {
        data->first->supprimerFavori(data->second);
    }
    delete data;
}


static void onBoutonFavorisClicked(GtkButton* button, gpointer user_data) {
    auto* navigateur = static_cast<Navigateur*>(user_data);
    if (navigateur) {
        navigateur->ajouterFavori("Favori", navigateur->getURLActuelle(), "");
    }
}

static void on_menu_item_activate(GtkWidget* widget, gpointer user_data) {
    auto* data = static_cast<std::pair<Navigateur*, std::string>*>(user_data);
    data->first->chargerURL(data->second);
    delete data;
}


static void on_favori_destroy(GtkWidget* widget, gpointer user_data) {
    g_free(user_data);
}

static void on_destroy_callback(GtkWidget* widget, gpointer user_data) {
    delete static_cast<std::pair<Navigateur*, std::string>*>(user_data);
}


static void on_ajouter_onglet(GtkButton* button, gpointer user_data) {
    auto* navigateur = static_cast<Navigateur*>(user_data);
    if (navigateur) {
        navigateur->ajouterNouvelOnglet(navigateur->getHomepage());
    }
}

static void on_page_chargee(GtkLabel* label, const std::string& titre) {
    gtk_label_set_text(label, titre.c_str());
}


static void on_naviguer_retour(GtkButton* button, gpointer user_data) {
    auto* navigateur = static_cast<Navigateur*>(user_data);
    navigateur->onNaviguerRetour(button, navigateur);
}

static void on_naviguer_suivant(GtkButton* button, gpointer user_data) {
    auto* navigateur = static_cast<Navigateur*>(user_data);
    navigateur->onNaviguerSuivant(button, navigateur);
}

static void on_aller_accueil(GtkButton* button, gpointer user_data) {
    auto* navigateur = static_cast<Navigateur*>(user_data);
    navigateur->onAllerAccueil(button, navigateur);
}

static void on_rafraichir_page(GtkButton* button, gpointer user_data) {
    auto* navigateur = static_cast<Navigateur*>(user_data);
    navigateur->onRafraichirPage(button, navigateur);
}
// Fonction statique pour gérer l'ouverture dans un nouvel onglet
static void on_ouvrir_nouvel_onglet(GtkWidget*, gpointer user_data) {
    auto* data = static_cast<std::pair<Navigateur*, std::string>*>(user_data);
    data->first->ajouterNouvelOnglet(data->second);
    delete data;
}
// Définition pour Utils.cpp
void on_modifier_favori(GtkWidget*, gpointer user_data) {
    auto* navigateur = static_cast<Navigateur*>(user_data);
    navigateur->afficherGestionnaireFavoris();
}


static void on_ajouter_favori_menu(GtkWidget*, gpointer user_data) {
    auto* data = static_cast<std::pair<Navigateur*, std::pair<GtkWidget*, GtkWidget*>>*>(user_data);
    auto* navigateur = data->first;
    GtkWidget* entryNom = data->second.first;
    GtkWidget* entryURL = data->second.second;

    const gchar* nom = gtk_entry_get_text(GTK_ENTRY(entryNom));
    const gchar* url = gtk_entry_get_text(GTK_ENTRY(entryURL));

    navigateur->ajouterFavori(nom, url, "Général");
    delete data;
}


static void on_bouton_favoris_clicked(GtkButton*, gpointer user_data) {
    auto* navigateur = static_cast<Navigateur*>(user_data);
    if (navigateur) {
        std::string url = navigateur->getURLActuelle();
        if (!url.empty()) {
            navigateur->ajouterFavori("Favori", url, "");
            navigateur->rafraichirBarreFavoris();
        }
    }
}

static void on_favori_clicked(GtkButton* button, gpointer user_data) {
    auto* data = static_cast<std::pair<Navigateur*, std::string>*>(user_data);
    if (data && data->first) {
        data->first->chargerURL(data->second);
        delete data;  // Libérer la mémoire allouée
    }
}

static void on_ajouter_groupe(GtkWidget* widget, gpointer data) {
    auto* info = static_cast<std::pair<Navigateur*, GtkWidget*>*>(data);
    if (info && info->first) {
        const gchar* nomGroupe = gtk_entry_get_text(GTK_ENTRY(info->second));
        info->first->getGestionnaireOnglets()->ajouterGroupe(nomGroupe);
        info->first->changerGroupeOnglets(nomGroupe);
        delete info;
    }
}

static void on_changer_groupe(GtkWidget* item, gpointer data) {
    auto* navigateur = static_cast<Navigateur*>(data);
    if (navigateur) {
        const char* nomGroupe = gtk_menu_item_get_label(GTK_MENU_ITEM(item));
        navigateur->changerGroupeOnglets(nomGroupe);
    }
}

static gboolean on_delete_event(GtkWidget*, GdkEvent*, gpointer user_data) {
    auto* navigateur = static_cast<Navigateur*>(user_data);
    navigateur->sauvegarderConfiguration();
    gtk_main_quit();
    return FALSE;
}

static gboolean on_key_press(GtkWidget*, GdkEvent* event, gpointer user_data) {
    auto* navigateur = static_cast<Navigateur*>(user_data);
    
    if (event->type == GDK_KEY_PRESS) {
        GdkEventKey* key_event = (GdkEventKey*) event;

        // Gestion du raccourci CTRL + T pour ouvrir un nouvel onglet
        if ((key_event->state & GDK_CONTROL_MASK) && key_event->keyval == GDK_KEY_t) {
            navigateur->ajouterNouvelOnglet(navigateur->getHomepage());
            return TRUE;
        }

        // Gestion du raccourci CTRL + D pour ajouter un favori
        if ((key_event->state & GDK_CONTROL_MASK) && key_event->keyval == GDK_KEY_d) {
            std::string url = navigateur->getURLActuelle();
            std::string titre = navigateur->getTitreActuel();
            
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
            navigateur->afficherPaletteCommandes();
            return TRUE;
        }
        
        // Gestion du raccourci CTRL + SHIFT + D pour dupliquer l'onglet actuel
        if ((key_event->state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) && 
            key_event->keyval == GDK_KEY_d) {
            std::string url = navigateur->getURLActuelle();
            if (!url.empty()) {
                navigateur->ajouterNouvelOnglet(url);
            }
            return TRUE;
        }
    }
    return FALSE;
}


// ✅ Gestion du clic droit (menu contextuel)
static gboolean on_favoris_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    if (event->type == GDK_BUTTON_PRESS && event->button == GDK_BUTTON_SECONDARY) {  // Clic droit détecté
        auto* navigateur = static_cast<Navigateur*>(user_data);
        if (!navigateur) return FALSE;

        // Créer un menu contextuel
        GtkWidget *menu = gtk_menu_new();
        
        // **Option 1 : Ouvrir dans un nouvel onglet**
        auto data = std::make_unique<std::pair<Navigateur*, std::string>>(navigateur, navigateur->getURLActuelle());
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
                         new std::pair<Navigateur*, GtkWidget*>(navigateur, widget),
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
    auto* navigateur = static_cast<Navigateur*>(user_data);

    // Récupérer les valeurs des champs
    const gchar* nom = gtk_entry_get_text(GTK_ENTRY(navigateur->getEntryNomFavori()));
    const gchar* url = gtk_entry_get_text(GTK_ENTRY(navigateur->getEntryURLFavori()));

    if (nom && url && *nom && *url) {
        // Ajout du favori dans la liste
        navigateur->ajouterFavori(nom, url, "Général");
        navigateur->rafraichirBarreFavoris();

        // Cacher le popover via la méthode publique
        gtk_widget_hide(navigateur->getPopoverFavoris());
    } else {
        std::cerr << "Veuillez remplir les deux champs." << std::endl;
    }
}




Navigateur::Navigateur() 
    : moteurRendu(std::make_unique<MoteurRendu>()),
      gestionnaireHTTP(std::make_unique<GestionnaireHTTP>()),
      gestionnaireMemoire(std::make_unique<GestionnaireMemoire>()),
      gestionnaireOnglets(std::make_unique<GestionnaireOnglets>()),
      moteurScript(std::make_unique<MoteurScript>()),
      favoris(std::make_shared<nlohmann::json>()),
      fenetre(nullptr),
      conteneurPrincipal(nullptr),
      barreNavigation(nullptr),
      barreFavoris(nullptr),
      barreOnglets(nullptr),
      barreURL(nullptr),
      boutonEtoile(nullptr),
      entryNomFavori(nullptr),
      entryURLFavori(nullptr),
      popoverFavoris(nullptr)
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
    
    gestionnaireFavoris = std::make_unique<GestionnaireFavoris>(favoris, [this]() { rafraichirBarreFavoris(); });
    chargerConfiguration();
    construireInterface();
    g_signal_connect(fenetre, "key-press-event", G_CALLBACK(on_key_press), this);
}


Navigateur::~Navigateur() {
    // Sauvegarder la configuration avant de fermer
    sauvegarderConfiguration();
    
    // Nettoyer les signaux avant de détruire les widgets
    if (fenetre && GTK_IS_WIDGET(fenetre)) {
        // Déconnecter tous les signaux de la fenêtre
        g_signal_handlers_disconnect_matched(fenetre, G_SIGNAL_MATCH_DATA, 0, 0, nullptr, nullptr, this);
        
        // Nettoyer le moteur de rendu (qui nettoie ses propres signaux)
        if (moteurRendu) {
            moteurRendu.reset();
        }
        
        // Détruire la fenêtre (cela détruira automatiquement tous les enfants)
        gtk_widget_destroy(fenetre);
        fenetre = nullptr;
    }
    
    // Réinitialiser les pointeurs pour éviter les accès après destruction
    conteneurPrincipal = nullptr;
    barreNavigation = nullptr;
    barreFavoris = nullptr;
    barreOnglets = nullptr;
    barreURL = nullptr;
    boutonEtoile = nullptr;
    entryNomFavori = nullptr;
    entryURLFavori = nullptr;
    popoverFavoris = nullptr;
}

std::shared_ptr<nlohmann::json> Navigateur::getFavoris() {
    return favoris;
}

void Navigateur::construireInterface() {
    std::cerr << "[DEBUG] Début de construireInterface()" << std::endl;
    fenetre = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    std::cerr << "[DEBUG] Fenêtre créée" << std::endl;
    gtk_window_set_title(GTK_WINDOW(fenetre), "WeedlyWeb");
    gtk_window_set_default_size(GTK_WINDOW(fenetre), 1024, 768);
    
    // Configuration pour que la fenêtre apparaisse dans la barre des tâches
    // Définir le nom de classe X11 pour l'identification par le gestionnaire de fenêtres
    gtk_widget_set_name(fenetre, "weedlyweb");
    
    // Définir le rôle de la fenêtre (pour le gestionnaire de fenêtres)
    gtk_window_set_role(GTK_WINDOW(fenetre), "weedlyweb-browser");
    
    // S'assurer que la fenêtre n'est pas ignorée par le gestionnaire de fenêtres
    // (par défaut, GTK_WINDOW_TOPLEVEL devrait déjà être visible, mais on s'en assure)
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(fenetre), FALSE);
    gtk_window_set_skip_pager_hint(GTK_WINDOW(fenetre), FALSE);
    
    // Définir le type de fenêtre (normal, pas un splash ou un popup)
    gtk_window_set_type_hint(GTK_WINDOW(fenetre), GDK_WINDOW_TYPE_HINT_NORMAL);
    
    std::cerr << "[DEBUG] Fenêtre configurée" << std::endl;

    // Connexion sécurisée du signal de fermeture avec lambda sécurisée
    g_signal_connect(fenetre, "delete-event", G_CALLBACK(+[](GtkWidget*, GdkEvent*, gpointer user_data) -> gboolean {
        auto* navigateur = static_cast<Navigateur*>(user_data);
        navigateur->sauvegarderConfiguration();
        gtk_main_quit();  // Quitter proprement l'application
        return FALSE;  
    }), this);

    // Conteneur principal
    std::cerr << "[DEBUG] Création du conteneur principal" << std::endl;
    conteneurPrincipal = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(fenetre), conteneurPrincipal);
    std::cerr << "[DEBUG] Conteneur principal ajouté à la fenêtre" << std::endl;

    // ORDRE CORRECT : Barres en haut, zone web en bas (expandable)
    // 1. Barre d'onglets (en haut)
    std::cerr << "[DEBUG] Initialisation de la barre d'onglets..." << std::endl;
    initialiserBarreOnglets();
    std::cerr << "[DEBUG] Barre d'onglets initialisée" << std::endl;
    
    // 2. Barre de navigation avec URL (sous les onglets)
    std::cerr << "[DEBUG] Initialisation de la barre de navigation..." << std::endl;
    initialiserBarreNavigation();
    std::cerr << "[DEBUG] Barre de navigation initialisée" << std::endl;
    
    // 3. Barre de favoris (optionnelle, sous la navigation)
    std::cerr << "[DEBUG] Initialisation de la barre de favoris..." << std::endl;
    initialiserBarreFavoris();
    std::cerr << "[DEBUG] Barre de favoris initialisée" << std::endl;
    
    // 4. Zone de rendu web (en bas, expandable)
    std::cerr << "[DEBUG] Initialisation du moteur de rendu..." << std::endl;
    moteurRendu->initialiserRendu(conteneurPrincipal);
    std::cerr << "[DEBUG] Moteur de rendu initialisé" << std::endl;
    
    // Afficher la fenêtre
    std::cerr << "[DEBUG] Affichage de la fenêtre..." << std::endl;
    gtk_widget_show_all(fenetre);
    
    // Présenter la fenêtre au gestionnaire de fenêtres (pour qu'elle apparaisse dans la barre des tâches)
    gtk_window_present(GTK_WINDOW(fenetre));
    
    std::cerr << "[DEBUG] Fenêtre affichée" << std::endl;
    
    // Ajouter le premier onglet et charger la page d'accueil
    std::cerr << "[DEBUG] Ajout du premier onglet..." << std::endl;
    std::string homepageUrl = homepage.empty() ? "https://www.duckduckgo.com" : homepage;
    ajouterNouvelOnglet(homepageUrl);
    std::cerr << "[DEBUG] Premier onglet ajouté avec URL: " << homepageUrl << std::endl;
    
    // Rafraîchir la barre de favoris pour qu'elle s'affiche
    rafraichirBarreFavoris();
    
    // Charger les styles CSS pour un design minimaliste
    chargerStyles();
    
    std::cerr << "[DEBUG] Fin de construireInterface()" << std::endl;

    moteurRendu->connecterSignalURLChangee([this](const std::string& url) {
        if (barreURL) {
            gtk_entry_set_text(GTK_ENTRY(barreURL), url.c_str());
        }
    });

    boutonEtoile = moteurRendu->creerBouton("☆", G_CALLBACK(on_bouton_favoris_clicked), this);
    gtk_box_pack_start(GTK_BOX(barreNavigation), boutonEtoile, FALSE, FALSE, 0);

    initialiserPopoverFavoris();
    // gtk_widget_show_all sera appelé après l'ajout de l'onglet
}

void Navigateur::ajouterBouton(GtkWidget* conteneur, const std::string& iconName, GCallback callback, gpointer data) {
    GtkWidget *bouton = moteurRendu->creerBouton(iconName, callback, data);
    if (!gtk_widget_get_parent(bouton)) {  // Éviter les duplications
        gtk_box_pack_start(GTK_BOX(conteneur), bouton, FALSE, FALSE, 0);
    }
}

std::string Navigateur::getTitreActuel() const {
    return moteurRendu->obtenirTitreActuel();
}



GtkWidget* obtenirDernierEnfant(GtkWidget* parent) {
    GList* enfants = gtk_container_get_children(GTK_CONTAINER(parent));
    return g_list_last(enfants) ? GTK_WIDGET(g_list_last(enfants)->data) : nullptr;
}

GtkWidget* obtenirPremierEnfant(GtkWidget* parent) {
    GList* enfants = gtk_container_get_children(GTK_CONTAINER(parent));
    return enfants ? GTK_WIDGET(enfants->data) : nullptr;
}

void Navigateur::initialiserBarreNavigation() {
    barreNavigation = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_widget_set_margin_start(barreNavigation, 5);
    gtk_widget_set_margin_end(barreNavigation, 5);
    gtk_widget_set_margin_top(barreNavigation, 5);
    gtk_widget_set_margin_bottom(barreNavigation, 5);
    gtk_widget_set_name(barreNavigation, "barre-navigation");

    // Boutons de navigation (retour, suivant, rafraîchir, accueil)
    ajouterBouton(barreNavigation, "go-previous", G_CALLBACK(on_naviguer_retour), this);
    ajouterBouton(barreNavigation, "go-next", G_CALLBACK(on_naviguer_suivant), this);
    ajouterBouton(barreNavigation, "view-refresh", G_CALLBACK(on_rafraichir_page), this);
    ajouterBouton(barreNavigation, "go-home", G_CALLBACK(on_aller_accueil), this);
    
    // Barre d'URL (expandable)
    barreURL = moteurRendu->creerChampTexte(G_CALLBACK(&Navigateur::onBarreURLActivate), this);
    gtk_box_pack_start(GTK_BOX(barreNavigation), barreURL, TRUE, TRUE, 0);

    // Bouton favoris (étoile)
    boutonEtoile = moteurRendu->creerBouton("☆", G_CALLBACK(on_bouton_favoris_clicked), this);
    gtk_box_pack_start(GTK_BOX(barreNavigation), boutonEtoile, FALSE, FALSE, 0);

    // Menu hamburger (trois barres horizontales) pour les options
    GtkWidget* boutonMenu = moteurRendu->creerBouton("open-menu", G_CALLBACK(+[](GtkButton*, gpointer user_data) {
        auto* navigateur = static_cast<Navigateur*>(user_data);
        navigateur->afficherMenuOptions();
    }), this);
    gtk_box_pack_start(GTK_BOX(barreNavigation), boutonMenu, FALSE, FALSE, 0);

    // Ajouter la barre de navigation au conteneur principal
    if (conteneurPrincipal && !gtk_widget_get_parent(barreNavigation)) {
        gtk_box_pack_start(GTK_BOX(conteneurPrincipal), barreNavigation, FALSE, FALSE, 0);
    }
}


std::string Navigateur::getURLActuelle() const {
    return moteurRendu->obtenirURLActuelle();
}


void Navigateur::initialiserBarreFavoris() {
    if (barreFavoris) {
        // Retirer du conteneur avant de détruire
        if (gtk_widget_get_parent(barreFavoris)) {
            gtk_container_remove(GTK_CONTAINER(gtk_widget_get_parent(barreFavoris)), barreFavoris);
        }
        gtk_widget_destroy(barreFavoris);
        barreFavoris = nullptr;
    }

    // Créer une barre de favoris minimaliste et élégante
    barreFavoris = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3);
    gtk_widget_set_margin_start(barreFavoris, 5);
    gtk_widget_set_margin_end(barreFavoris, 5);
    gtk_widget_set_margin_top(barreFavoris, 2);
    gtk_widget_set_margin_bottom(barreFavoris, 2);
    
    // Style minimaliste pour la barre de favoris
    gtk_widget_set_name(barreFavoris, "barre-favoris");

    // Afficher les favoris (maximum 10 visibles, le reste dans un menu)
    int maxFavorisVisibles = 10;
    int compteur = 0;

    for (const auto& favori : *favoris) {
        if (compteur >= maxFavorisVisibles) {
            // Bouton "..." pour afficher les favoris restants
            GtkWidget* boutonPlus = gtk_button_new_with_label("⋯");
            gtk_widget_set_tooltip_text(boutonPlus, "Plus de favoris");
            gtk_widget_set_margin_start(boutonPlus, 2);
            gtk_widget_set_margin_end(boutonPlus, 2);
            g_signal_connect(boutonPlus, "clicked", G_CALLBACK(+[](GtkButton*, gpointer user_data) {
                auto* navigateur = static_cast<Navigateur*>(user_data);
                navigateur->afficherMenuFavorisRestants();
            }), this);
            gtk_box_pack_start(GTK_BOX(barreFavoris), boutonPlus, FALSE, FALSE, 0);
            break;
        }

        // Créer un bouton de favori avec un style minimaliste
        std::string nomFavori = favori.value("name", "Favori");
        // Limiter la longueur du nom pour un design propre
        if (nomFavori.length() > 15) {
            nomFavori = nomFavori.substr(0, 12) + "...";
        }
        
        GtkWidget* boutonFavori = gtk_button_new_with_label(nomFavori.c_str());
        gtk_widget_set_tooltip_text(boutonFavori, favori.value("url", "").c_str());
        gtk_widget_set_margin_start(boutonFavori, 2);
        gtk_widget_set_margin_end(boutonFavori, 2);
        
        auto* data = new std::pair<Navigateur*, std::string>(this, favori["url"]);
        g_signal_connect(boutonFavori, "clicked", G_CALLBACK(on_favori_clicked), data);
        
        // Style minimaliste pour les boutons de favoris
        gtk_widget_set_name(boutonFavori, "bouton-favori");
        
        gtk_box_pack_start(GTK_BOX(barreFavoris), boutonFavori, FALSE, FALSE, 0);
        compteur++;
    }

    // Si aucun favori, afficher un message discret
    if (favoris->empty()) {
        GtkWidget* labelVide = gtk_label_new("");
        gtk_widget_set_opacity(labelVide, 0.0); // Invisible mais prend de l'espace
        gtk_box_pack_start(GTK_BOX(barreFavoris), labelVide, FALSE, FALSE, 0);
    }

    // Ajouter la barre de favoris au conteneur principal
    if (conteneurPrincipal && !gtk_widget_get_parent(barreFavoris)) {
        gtk_box_pack_start(GTK_BOX(conteneurPrincipal), barreFavoris, FALSE, FALSE, 0);
    }
    gtk_widget_show_all(barreFavoris);
}

void Navigateur::afficherMenuFavoris() {
    GtkWidget* menu = gtk_menu_new();

    int largeurDispo = gtk_widget_get_allocated_width(conteneurPrincipal);
    int largeurActuelle = 0;


    for (const auto& favori : *favoris) {
        int largeurBouton = 80; // Estimation
        if (largeurActuelle + largeurBouton > largeurDispo) {
            GtkWidget* item = gtk_menu_item_new_with_label(favori["name"].get<std::string>().c_str());
            auto* data = new std::pair<Navigateur*, std::string>(this, favori["url"]);
            g_signal_connect(item, "activate", G_CALLBACK(on_menu_item_activate), data);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        }
        largeurActuelle += largeurBouton;
    }

    gtk_widget_show_all(menu);
    gtk_menu_popup_at_widget(GTK_MENU(menu), barreFavoris, GDK_GRAVITY_SOUTH, GDK_GRAVITY_NORTH, nullptr);
}

void Navigateur::afficherMenuFavorisRestants() {
    GtkWidget* menu = gtk_menu_new();
    int largeurDispo = gtk_widget_get_allocated_width(conteneurPrincipal);
    int largeurActuelle = 0;

    for (const auto& favori : *favoris) {
        int largeurBouton = 80; // Estimation de la largeur d'un bouton
        if (largeurActuelle + largeurBouton > largeurDispo) {
            GtkWidget* item = gtk_menu_item_new_with_label(favori["name"].get<std::string>().c_str());
            auto* data = new std::pair<Navigateur*, std::string>(this, favori["url"]);
            g_signal_connect(item, "activate", G_CALLBACK(on_menu_item_activate), data);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        }
        largeurActuelle += largeurBouton;
    }

    gtk_widget_show_all(menu);
    gtk_menu_popup_at_widget(GTK_MENU(menu), barreFavoris, GDK_GRAVITY_SOUTH, GDK_GRAVITY_NORTH, nullptr);
}


void Navigateur::afficherGestionnaireFavoris() {
    if (!gestionnaireFavoris) {
        gestionnaireFavoris = std::make_unique<GestionnaireFavoris>(favoris, [this]() { rafraichirBarreFavoris(); });
    }
    gestionnaireFavoris->afficherFenetre();
}


void Navigateur::rafraichirBarreFavoris() {
    if (GTK_IS_WIDGET(barreFavoris)) {
        // Retirer du conteneur avant de détruire
        if (gtk_widget_get_parent(barreFavoris)) {
            gtk_container_remove(GTK_CONTAINER(gtk_widget_get_parent(barreFavoris)), barreFavoris);
        }
        gtk_widget_destroy(barreFavoris);
        barreFavoris = nullptr;
    }

    barreFavoris = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

    if (favoris->empty()) {
        GtkWidget *labelAucunFavori = gtk_label_new("Aucun favori");
        gtk_box_pack_start(GTK_BOX(barreFavoris), labelAucunFavori, FALSE, FALSE, 5);
    }

    for (const auto& favori : *favoris) {
        GtkWidget *boutonFavori = moteurRendu->creerBouton(favori["name"], nullptr, nullptr);
        auto* data = new std::pair<Navigateur*, std::string>(this, favori["url"]);
        g_signal_connect(boutonFavori, "clicked", G_CALLBACK(on_favori_clicked), data);
        if (gtk_widget_get_parent(boutonFavori) == nullptr) {
            gtk_box_pack_start(GTK_BOX(barreFavoris), boutonFavori, FALSE, FALSE, 5);
        }
    }

    // Vérifier que barreFavoris n'est pas déjà dans le conteneur
    if (conteneurPrincipal && !gtk_widget_get_parent(barreFavoris)) {
        gtk_box_pack_start(GTK_BOX(conteneurPrincipal), barreFavoris, FALSE, FALSE, 5);
    }
    gtk_widget_show_all(barreFavoris);
}


void Navigateur::mettreAJourBoutonEtoile() {
    std::string urlActuelle = getURLActuelle();
    bool estDejaFavori = std::any_of(
        favoris->begin(), favoris->end(),
        [&urlActuelle](const nlohmann::json& favori) { return favori["url"] == urlActuelle; }
    );


    const char* symbole = estDejaFavori ? "★" : "☆";
    gtk_button_set_label(GTK_BUTTON(boutonEtoile), symbole);
}


void Navigateur::initialiserPopoverFavoris() {
    //popoverFavoris = gtk_popover_new(barreFavoris); // Attaché à la barre de favoris
    popoverFavoris = gtk_popover_new(barreNavigation); // Attaché à la barre
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    // Champs de saisie
    entryNomFavori = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entryNomFavori), "Nom du favori");

    entryURLFavori = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entryURLFavori), "URL du favori");

    // Boutton de validation
    GtkWidget* boutonAjouter = gtk_button_new_with_label("Ajouter Favori");
    g_signal_connect(boutonAjouter, "clicked", G_CALLBACK(on_button_ajouter_clicked), this);

    // Ajout des éléments dans la boîte
    gtk_box_pack_start(GTK_BOX(box), entryNomFavori, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(box), entryURLFavori, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(box), boutonAjouter, FALSE, FALSE, 5);
    
    // Ajouter le contenu au popover
    gtk_container_add(GTK_CONTAINER(popoverFavoris), box);
    // Ne pas afficher le popover automatiquement - il sera affiché uniquement quand l'utilisateur le demande
    // gtk_widget_show_all(popoverFavoris); // Retiré pour éviter l'affichage au démarrage
}


void Navigateur::initialiserBarreOnglets() {
    // Nettoyer l'ancienne barre d'onglets si elle existe
    if (barreOnglets && GTK_IS_WIDGET(barreOnglets)) {
        GtkWidget* parent = gtk_widget_get_parent(barreOnglets);
        if (parent && GTK_IS_CONTAINER(parent)) {
            gtk_container_remove(GTK_CONTAINER(parent), barreOnglets);
        }
        gtk_widget_destroy(barreOnglets);
        barreOnglets = nullptr;
    }

    // Créer une nouvelle barre d'onglets simple et propre
    barreOnglets = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_margin_start(barreOnglets, 5);
    gtk_widget_set_margin_end(barreOnglets, 5);
    gtk_widget_set_margin_top(barreOnglets, 5);
    gtk_widget_set_name(barreOnglets, "barre-onglets");
    
    // Bouton simple "+" pour ajouter un nouvel onglet (icône plus simple)
    GtkWidget* boutonAjouterOnglet = gtk_button_new_with_label("+");
    gtk_widget_set_tooltip_text(boutonAjouterOnglet, "Nouvel onglet");
    g_signal_connect(boutonAjouterOnglet, "clicked", G_CALLBACK(on_ajouter_onglet), this);
    gtk_box_pack_start(GTK_BOX(barreOnglets), boutonAjouterOnglet, FALSE, FALSE, 0);
    
    // Ajouter la barre d'onglets au conteneur principal (en haut)
    if (conteneurPrincipal && !gtk_widget_get_parent(barreOnglets)) {
        gtk_box_pack_start(GTK_BOX(conteneurPrincipal), barreOnglets, FALSE, FALSE, 0);
    }
}

void Navigateur::changerGroupeOnglets(const std::string& nomGroupe) {
    gestionnaireOnglets->changerGroupeActif(nomGroupe);
    // Retirer du conteneur avant de détruire
    if (barreOnglets && gtk_widget_get_parent(barreOnglets)) {
        gtk_container_remove(GTK_CONTAINER(gtk_widget_get_parent(barreOnglets)), barreOnglets);
    }
    if (barreOnglets) {
        gtk_widget_destroy(barreOnglets);
        barreOnglets = nullptr;
    }
    initialiserBarreOnglets();
}


void Navigateur::ajouterNouvelOnglet(const std::string &url) {
    std::cerr << "[DEBUG] Début de ajouterNouvelOnglet(" << url << ")" << std::endl;
    gestionnaireOnglets->ajouterOnglet(gestionnaireOnglets->getGroupeActif(), url);
    std::cerr << "[DEBUG] Onglet ajouté au gestionnaire" << std::endl;

    GtkWidget *hboxOnglet = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_widget_set_margin_start(hboxOnglet, 2);
    gtk_widget_set_margin_end(hboxOnglet, 2);
    
    // Label avec le titre de l'onglet (ou "Nouvel onglet" par défaut)
    GtkWidget *labelTitre = gtk_label_new("Nouvel onglet");
    gtk_box_pack_start(GTK_BOX(hboxOnglet), labelTitre, FALSE, FALSE, 5);

    // Bouton fermer (X)
    GtkWidget *boutonFermer = gtk_button_new_with_label("×");
    gtk_widget_set_tooltip_text(boutonFermer, "Fermer l'onglet");
    gtk_widget_set_margin_start(boutonFermer, 5);
    gtk_widget_set_margin_end(boutonFermer, 5);
    g_signal_connect(boutonFermer, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer user_data) {
        auto* n = static_cast<Navigateur*>(user_data);
        GtkWidget *parent = gtk_widget_get_parent(GTK_WIDGET(button));
        if (parent) {
            n->supprimerOnglet(parent);
        }
    }), this);
    gtk_box_pack_start(GTK_BOX(hboxOnglet), boutonFermer, FALSE, FALSE, 0);

    // Rendre l'onglet cliquable pour changer d'onglet actif
    gtk_widget_set_events(hboxOnglet, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(hboxOnglet, "button-press-event", G_CALLBACK(+[](GtkWidget* widget, GdkEventButton*, gpointer user_data) {
        auto* n = static_cast<Navigateur*>(user_data);
        if (n) {
            n->changerOngletActif(widget);
        }
        return TRUE;
    }), this);

    // Ajouter l'onglet à la barre d'onglets
    if (barreOnglets && !gtk_widget_get_parent(hboxOnglet)) {
        gtk_box_pack_start(GTK_BOX(barreOnglets), hboxOnglet, FALSE, FALSE, 0);
    }
    onglets.push_back({url, hboxOnglet});
    
    // Afficher l'onglet
    gtk_widget_show_all(hboxOnglet);
    
    // Changer l'onglet actif vers celui-ci
    changerOngletActif(hboxOnglet);
    
    // Afficher la page
    std::cerr << "[DEBUG] Affichage de la page: " << url << std::endl;
    if (moteurRendu && !url.empty()) {
        moteurRendu->afficherPage(url);
        std::cerr << "[DEBUG] Page affichée" << std::endl;
    }
    
    // Connecter le signal pour mettre à jour le titre de l'onglet quand la page se charge
    if (moteurRendu) {
        moteurRendu->connecterSignalPageChargee([labelTitre](const std::string &titre) {
            if (labelTitre && GTK_IS_LABEL(labelTitre)) {
                std::string titreCourt = titre.length() > 20 ? titre.substr(0, 17) + "..." : titre;
                gtk_label_set_text(GTK_LABEL(labelTitre), titreCourt.c_str());
            }
        });
    }
    
    std::cerr << "[DEBUG] Fin de ajouterNouvelOnglet()" << std::endl;


    gtk_widget_show_all(barreOnglets);
}

void Navigateur::changerOngletActif(GtkWidget* ongletWidget) {
    for (auto &[url, widget] : onglets) {
        if (widget == ongletWidget) {
            moteurRendu->afficherPage(url);
            mettreEnSurbrillance(widget);
            return;
        }
    }
}



void Navigateur::executerScriptDansOngletActif(const std::string& script) {
    if (!onglets.empty()) {
        GtkWidget* ongletActif = onglets.back().second;
        WebKitWebView* vueWeb = nullptr;
        GList* children = gtk_container_get_children(GTK_CONTAINER(ongletActif));
        if (children != nullptr) {
            vueWeb = WEBKIT_WEB_VIEW(children->data);
            g_list_free(children);
        }        
        moteurScript->executerScript(vueWeb, script);
    }
}

void Navigateur::supprimerFavori(GtkWidget* widget) {
    const gchar* nomFavori = gtk_button_get_label(GTK_BUTTON(widget));
    gestionnaireFavoris->supprimerFavori(nomFavori);
    rafraichirBarreFavoris();  // Mise à jour visuelle
}


void Navigateur::supprimerOnglet(GtkWidget *ongletWidget) {
    // Trouver l'onglet à supprimer
    auto it = std::find_if(onglets.begin(), onglets.end(), [ongletWidget](const auto &pair) {
        return pair.second == ongletWidget;
    });

    if (it != onglets.end()) {
        GtkWidget *hboxOnglet = it->second;

        WebKitWebView* vueWeb = nullptr;  // Déclaration ici

        GList* enfants = gtk_container_get_children(GTK_CONTAINER(hboxOnglet));
        if (enfants != nullptr) {
            for (GList* iter = enfants; iter != nullptr; iter = iter->next) {
                if (WEBKIT_IS_WEB_VIEW(iter->data)) {
                    vueWeb = WEBKIT_WEB_VIEW(iter->data);
                    break;
                }
            }
            g_list_free(enfants);
        }

        if (vueWeb && WEBKIT_IS_WEB_VIEW(vueWeb)) {
            gestionnaireMemoire->hibernerOnglet(vueWeb); // Mise en veille de l'onglet
        }

        // Supprimer l'onglet de la liste
        gtk_widget_destroy(hboxOnglet);
        onglets.erase(it);

        // Si aucun onglet n'est présent, fermer l'application
        if (onglets.empty()) {
            fermerApplication();
        } else {
            // Charger l'URL du premier onglet
            chargerURL(onglets.front().first);
        }
    }
}

void Navigateur::mettreEnSurbrillance(GtkWidget *ongletWidget) {
    for (auto &[url, widget] : onglets) {
        gtk_widget_set_name(widget, widget == ongletWidget ? "onglet-actif": "onlget-inactif");
    }
}

void Navigateur::chargerURL(const std::string& url) {
    moteurRendu->afficherPage(url);
    historique.push_back(url);

    if (barreURL) {
        gtk_entry_set_text(GTK_ENTRY(barreURL), url.c_str());
    }

    // Mettre à jour le titre de l'onglet actif
    moteurRendu->connecterSignalTitreChange([this](const std::string& titre) {
        if (!onglets.empty()) {
            GtkWidget* hboxOnglet = onglets.back().second;
            GList* enfants = gtk_container_get_children(GTK_CONTAINER(hboxOnglet));
            if (enfants) {
                GtkWidget* labelTitre = GTK_WIDGET(enfants->data);
                if (GTK_IS_LABEL(labelTitre)) {
                    gtk_label_set_text(GTK_LABEL(labelTitre), titre.c_str());
                }
                g_list_free(enfants);
            }
        }
    });
    gestionnaireMemoire->optimiserMemoire();
}

void Navigateur::afficherMessage(const std::string& message) {
    std::cout << "Message : " << message << std::endl;
}


void Navigateur::chargerConfiguration() {
    std::string chemin = GestionnaireFichiers::cheminConfigJSON();
    nlohmann::json config = GestionnaireFichiers::lireJSON(chemin);
    
    std::string cheminFavoris = GestionnaireFichiers::cheminFavorisJSON();
    nlohmann::json favorisJson = GestionnaireFichiers::lireJSON(cheminFavoris);

    if (favorisJson.is_null() || favorisJson.empty()) {
        std::cerr << "Aucun favori trouvé, initialisation avec un favori par défaut." << std::endl;
        favorisJson = nlohmann::json::array({
            {{"name", "DuckDuckGo"}, {"url", "https://www.duckduckgo.com"}, {"tag", "Recherche"}}
        });
        GestionnaireFichiers::ecrireJSON(cheminFavoris, favorisJson);
    }
    
    // Assigner les favoris au membre de la classe
    *favoris = favorisJson;
    
    if (config.is_null() || config.empty()) {
        std::cerr << "Fichier de configuration non trouvé ou vide. Création d'une configuration par défaut." << std::endl;
        config["homepage"] = "https://www.duckduckgo.com";
        GestionnaireFichiers::ecrireJSON(chemin, config);
    }

    if (config.contains("onglets")) {
        for (const auto& onglet : config["onglets"]) {
            if (onglet.contains("url")) {
                ajouterNouvelOnglet(onglet["url"]);
            }
        }
    }
    
    homepage = config.value("homepage", "https://www.duckduckgo.com");
    std::cout << "Page d'accueil définie sur : " << homepage << std::endl;
}


void Navigateur::sauvegarderConfiguration() {
    nlohmann::json config;
    config["homepage"] = homepage;
    GestionnaireFichiers::ecrireJSON(GestionnaireFichiers::cheminConfigJSON(), config);
    
    GestionnaireFichiers::ecrireJSON(GestionnaireFichiers::cheminFavorisJSON(), *favoris);
    std::cout << "Favoris sauvegardés automatiquement !" << std::endl;
    
    nlohmann::json ongletsJson = nlohmann::json::array();
    for (const auto& [url, widget] : onglets) {
        ongletsJson.push_back({{"url", url}});
    }
    config["onglets"] = ongletsJson;
}

void Navigateur::ajouterFavori(const std::string& nom, const std::string& url, const std::string& tag) {
    gestionnaireFavoris->ajouterFavori(nom, url, tag);
    rafraichirBarreFavoris();
}



void Navigateur::afficherMenuOptions() {
    GtkWidget* menu = gtk_menu_new();
    
    // Option : Gestionnaire de favoris
    GtkWidget* itemFavoris = gtk_menu_item_new_with_label("Gestionnaire de Favoris");
    g_signal_connect(itemFavoris, "activate", G_CALLBACK(+[](GtkWidget*, gpointer user_data) {
        auto* navigateur = static_cast<Navigateur*>(user_data);
        navigateur->afficherGestionnaireFavoris();
    }), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), itemFavoris);
    
    // Séparateur
    GtkWidget* separator1 = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), separator1);
    
    // Option : Paramètres
    GtkWidget* itemParametres = gtk_menu_item_new_with_label("Paramètres");
    g_signal_connect(itemParametres, "activate", G_CALLBACK(+[](GtkWidget*, gpointer user_data) {
        auto* navigateur = static_cast<Navigateur*>(user_data);
        navigateur->afficherParametres();
    }), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), itemParametres);
    
    // Option : À propos
    GtkWidget* itemAPropos = gtk_menu_item_new_with_label("À propos");
    g_signal_connect(itemAPropos, "activate", G_CALLBACK(+[](GtkWidget*, gpointer user_data) {
        GtkWidget* dialog = gtk_message_dialog_new(
            nullptr,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_INFO,
            GTK_BUTTONS_OK,
            "WeedlyWeb\n\nNavigateur web moderne basé sur WebKit2GTK\nVersion 1.0"
        );
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }), nullptr);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), itemAPropos);
    
    // Séparateur
    GtkWidget* separator2 = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), separator2);
    
    // Option : Quitter
    GtkWidget* itemQuitter = gtk_menu_item_new_with_label("Quitter");
    g_signal_connect(itemQuitter, "activate", G_CALLBACK(+[](GtkWidget*, gpointer user_data) {
        auto* navigateur = static_cast<Navigateur*>(user_data);
        navigateur->fermerApplication();
    }), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), itemQuitter);
    
    gtk_widget_show_all(menu);
    
    // Trouver le bouton menu pour positionner le popup
    GList* children = gtk_container_get_children(GTK_CONTAINER(barreNavigation));
    GtkWidget* boutonMenu = nullptr;
    for (GList* iter = children; iter; iter = iter->next) {
        GtkWidget* widget = GTK_WIDGET(iter->data);
        if (GTK_IS_BUTTON(widget)) {
            const gchar* icon = gtk_button_get_image(GTK_BUTTON(widget)) ? 
                gtk_image_get_icon_name(GTK_IMAGE(gtk_button_get_image(GTK_BUTTON(widget)))) : nullptr;
            if (icon && g_strcmp0(icon, "open-menu") == 0) {
                boutonMenu = widget;
                break;
            }
        }
    }
    g_list_free(children);
    
    if (boutonMenu) {
        gtk_menu_popup_at_widget(GTK_MENU(menu), boutonMenu, GDK_GRAVITY_SOUTH_WEST, GDK_GRAVITY_NORTH_WEST, nullptr);
    } else {
        gtk_menu_popup_at_pointer(GTK_MENU(menu), nullptr);
    }
}

void Navigateur::afficherParametres() {
    std::string cheminParametres = GestionnaireFichiers::cheminParametresHTML();
    moteurRendu->afficherPage("file://" + cheminParametres);
}


void Navigateur::configurerRaccourcisClavier() {
    g_signal_connect(fenetre, "key-press-event", G_CALLBACK(+[](GtkWidget *, GdkEvent *event, gpointer user_data) {
        auto* navigateur = static_cast<Navigateur*>(user_data);

        if (event->type == GDK_KEY_PRESS) {
            GdkEventKey* key_event = (GdkEventKey*) event;
            
            if ((key_event->state & GDK_CONTROL_MASK) && key_event->keyval == GDK_KEY_t) {
                navigateur->ajouterNouvelOnglet(navigateur->getHomepage());
            } else if ((key_event->state & GDK_CONTROL_MASK) && key_event->keyval == GDK_KEY_w) {
                if (!navigateur->onglets.empty()) {
                    navigateur->supprimerOnglet(navigateur->onglets.back().second);
                }
            } else if ((key_event->state & GDK_CONTROL_MASK) && key_event->keyval == GDK_KEY_d) {
                std::string url = navigateur->moteurRendu->obtenirURLActuelle();
                if (!url.empty()) {
                    navigateur->gestionnaireFavoris->ajouterFavori("Favori", url, "");
                    navigateur->rafraichirBarreFavoris();
                }
            } else if ((key_event->state & GDK_CONTROL_MASK) && key_event->keyval == GDK_KEY_e) {
                // Focus sur la barre d'URL
                if (navigateur->barreURL) {
                    gtk_widget_grab_focus(navigateur->barreURL);
                    gtk_editable_set_position(GTK_EDITABLE(navigateur->barreURL), -1); // Place le curseur a la fin
                }
            }
        }
        return FALSE;
    }), this);
}


void Navigateur::chargerStyles() {
    GtkCssProvider *provider = gtk_css_provider_new();
    GError *error = NULL;

    // CSS minimaliste et moderne
    const gchar* css = 
        "#barre-favoris { background-color: rgba(240, 240, 240, 0.5); border-bottom: 1px solid rgba(0, 0, 0, 0.1); padding: 4px; } "
        "#bouton-favori { border: none; border-radius: 4px; padding: 4px 8px; background-color: rgba(255, 255, 255, 0.8); } "
        "#bouton-favori:hover { background-color: rgba(220, 220, 220, 0.9); } "
        "#bouton-favori:active { background-color: rgba(200, 200, 200, 1.0); } "
        "#barre-navigation { padding: 6px; border-bottom: 1px solid rgba(0, 0, 0, 0.1); } "
        "#barre-onglets { padding: 4px; border-bottom: 1px solid rgba(0, 0, 0, 0.1); } "
        ".onglet { border-radius: 4px 4px 0 0; padding: 6px 12px; margin: 0 2px; background-color: rgba(240, 240, 240, 0.8); } "
        ".onglet:hover { background-color: rgba(220, 220, 220, 0.9); } "
        ".onglet-actif { background-color: rgba(255, 255, 255, 1.0); border-bottom: 2px solid #4A90E2; }";

    gtk_css_provider_load_from_data(provider, css, -1, &error);
    
    if (error) {
        g_warning("Erreur de chargement du CSS : %s", error->message);
        g_error_free(error);
        g_object_unref(provider);
        return;
    }

    // Appliquer le style à l'écran
    GtkStyleContext *context = gtk_widget_get_style_context(fenetre);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    
    // Ne pas libérer le provider ici - il sera libéré automatiquement par GTK 
        "#barre-favoris { background-color: #f0f0f0; padding: 5px; }", 
        -1, nullptr);

    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_USER
    );
    g_object_unref(provider);


}


void Navigateur::onNaviguerRetour(GtkButton *, Navigateur *n) {
    n->moteurRendu->naviguerRetour();
}

void Navigateur::onNaviguerSuivant(GtkButton *, Navigateur *n) {
    n->moteurRendu->naviguerSuivant();
}

void Navigateur::onRafraichirPage(GtkButton *, Navigateur *n) {
    n->chargerURL(n->moteurRendu->obtenirURLActuelle());
}

void Navigateur::onAllerAccueil(GtkButton *, Navigateur *n) {
    n->chargerURL(n->homepage);
}



void Navigateur::onBarreURLActivate(GtkEntry* entry, gpointer user_data) {
    auto* navigateur = static_cast<Navigateur*>(user_data);
    std::string url = gtk_entry_get_text(entry);
    navigateur->chargerURL(url);
}

void Navigateur::fermerApplication() {
    sauvegarderConfiguration();
    if (GTK_IS_WIDGET(fenetre)) {
        gtk_widget_destroy(fenetre);
        fenetre = nullptr;
    }
    gtk_main_quit();
}

void Navigateur::onNaviguerRetourWrapper(GtkButton* button, gpointer user_data) {
    auto* navigateur = static_cast<Navigateur*>(user_data);
    navigateur->onNaviguerRetour(button, navigateur);
}



void Navigateur::onNaviguerSuivantWrapper(GtkButton *button, gpointer user_data) {
    static_cast<Navigateur*>(user_data)->onNaviguerSuivant(button, static_cast<Navigateur*>(user_data));
}

void Navigateur::onRafraichirPageWrapper(GtkButton *button, gpointer user_data) {
    static_cast<Navigateur*>(user_data)->onRafraichirPage(button, static_cast<Navigateur*>(user_data));
}
void Navigateur::creerMenuContextuel(GtkWidget* bouton) {
    GtkWidget *menu = gtk_menu_new();

    // Option "Ajouter aux Favoris"
    GtkWidget *ajouterItem = gtk_menu_item_new_with_label("Ajouter aux Favori");
    g_signal_connect(ajouterItem, "activate", G_CALLBACK(on_ajouter_favori_menu), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), ajouterItem);
    
    // Option "Ouvrir un nouvel onglet"
    GtkWidget *nouvelOngletItem = gtk_menu_item_new_with_label("Ouvrir un nouvel onglet");
    g_signal_connect(nouvelOngletItem, "activate", G_CALLBACK(on_ajouter_onglet), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), nouvelOngletItem);

    auto* data = new std::pair<Navigateur*, GtkWidget*>(this, bouton);
    // Option "Supprimer le favori" (Ajout si applicable)
    GtkWidget *supprimerItem = gtk_menu_item_new_with_label("Supprimer le favori");
    g_signal_connect_data(supprimerItem, "activate", G_CALLBACK(on_supprimer_favori),
                      data, delete_user_data, G_CONNECT_AFTER);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), supprimerItem);

    gtk_widget_show_all(menu);  // Ajout avant l'ouverture du menu
    gtk_menu_popup_at_widget(GTK_MENU(menu), bouton, GDK_GRAVITY_SOUTH, GDK_GRAVITY_NORTH, nullptr);
}


void Navigateur::onBoutonFavorisClicked(GtkButton* button, gpointer user_data) {
    auto* navigateur = static_cast<Navigateur*>(user_data);
    if (!navigateur) return;

    // Obtenir l'URL actuelle et pré-remplir l'entrée
    std::string urlActuelle = navigateur->getURLActuelle();
    gtk_entry_set_text(GTK_ENTRY(navigateur->entryURLFavori), urlActuelle.c_str());

    // Vérifier si l'URL est déjà dans les favoris
    auto favoris = navigateur->getFavoris();
    bool estDejaFavori = std::any_of(
        favoris->begin(),
        favoris->end(),
        [&urlActuelle](const nlohmann::json& favori) { return favori["url"] == urlActuelle; }
    );

    // Si l'URL est déjà un favori, afficher un message et ne pas ouvrir le popover
    if (estDejaFavori) {
        std::cerr << "URL déjà ajoutée aux favoris : " << urlActuelle << std::endl;
        gtk_button_set_label(button, "*"); // Marquer l'URL comme déjà en favoris
        navigateur->afficherGestionnaireFavoris(); // Permet la modification
    } else {
        // Sinon, ouvrir le popover pour permettre l'ajout
        gtk_button_set_label(button, "☆");  // Étoile vide
        gtk_popover_popup(GTK_POPOVER(navigateur->getPopoverFavoris()));
    }
}

void Navigateur::afficherPaletteCommandes() {
    if (commandPalette && fenetre) {
        commandPalette->setCurrentWebView(moteurRendu.get());
        commandPalette->showPalette(GTK_WINDOW(fenetre));
    }
}

// Fonctions hibernerOnglet et reactiverOnglet supprimées - gérées par GestionnaireMemoire
