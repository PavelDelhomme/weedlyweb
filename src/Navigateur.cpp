#include "Navigateur.h"
#include "GestionnaireFichiers.h"
#include "GestionnaireMemoire.h"
#include "GestionnaireFavoris.h"
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
static void on_ouvrir_nouvel_onglet_safe(GtkWidget*, gpointer user_data);

static void on_modifier_favori(GtkWidget*, gpointer user_data);

static void on_supprimer_favori(GtkWidget*, gpointer user_data) {
    auto* data = static_cast<std::pair<Navigateur*, GtkWidget*>*>(user_data);
    if (data && data->first) {
        data->first->supprimerFavori(data->second);
    }
    delete data;
}

static gboolean on_favori_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    if (event->type == GDK_BUTTON_PRESS && event->button == GDK_BUTTON_SECONDARY) {
        auto* navigateur = static_cast<Navigateur*>(user_data);
        GtkWidget* menu = creerMenuContextuelFavoris(navigateur, widget);
        gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent*)event);
        return TRUE;
    }
    return FALSE;
}



static void on_ouvrir_nouvel_onglet_safe(GtkWidget*, gpointer user_data) {
    auto* data = static_cast<std::pair<Navigateur*, std::string>*>(user_data);
    if (!data) return;
    if (data->first) {
        data->first->ajouterNouvelOnglet(data->second);
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
// Fonction statique pour gérer la modification du favori
static void on_modifier_favori(GtkWidget*, gpointer user_data) {
    auto* data = static_cast<std::pair<Navigateur*, GtkWidget*>*>(user_data);
    const gchar* nomFavori = gtk_button_get_label(GTK_BUTTON(data->second));
    const gchar* nouvelURL = "https://nouveau-lien.com"; // Exemple - récupéré via une boîte de dialogue
    data->first->gestionnaireFavoris->modifierFavori(nomFavori, nouvelURL);
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
            gtk_entry_set_text(GTK_ENTRY(navigateur->entryNomFavori), titre.c_str());
            gtk_entry_set_text(GTK_ENTRY(navigateur->entryURLFavori), url.c_str());

            // Afficher le formulaire d'ajout de favori (popover)
            gtk_popover_popup(GTK_POPOVER(navigateur->popoverFavoris));
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
      favoris(std::make_shared<nlohmann::json>())
{
    // Initialiser la base de données
    database = std::make_unique<Database>();
    if (!database->initDatabase()) {
        std::cerr << "Erreur lors de l'initialisation de la base de données" << std::endl;
    }
    
    // Initialiser l'intercepteur de requêtes
    requestInterceptor = std::make_unique<RequestInterceptor>();
    requestInterceptor->enable();
    
    // Initialiser la palette de commandes
    commandPalette = std::make_unique<CommandPalette>();
    commandPalette->setRequestInterceptor(requestInterceptor.get());
    
    gestionnaireFavoris = std::make_unique<GestionnaireFavoris>(favoris, [this]() { rafraichirBarreFavoris(); });
    chargerConfiguration();
    construireInterface();
    g_signal_connect(fenetre, "key-press-event", G_CALLBACK(on_key_press), this);
}


Navigateur::~Navigateur() {
    if (GTK_IS_WIDGET(fenetre)) {
        gtk_widget_destroy(fenetre);
        fenetre = nullptr;
    }
}

std::shared_ptr<nlohmann::json> Navigateur::getFavoris() {
    return favoris;
}

void Navigateur::construireInterface() {
    fenetre = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(fenetre), "WeedlyWeb");
    gtk_window_set_default_size(GTK_WINDOW(fenetre), 1024, 768);

    // Connexion sécurisée du signal de fermeture avec lambda sécurisée
    g_signal_connect(fenetre, "delete-event", G_CALLBACK(+[](GtkWidget*, GdkEvent*, gpointer user_data) -> gboolean {
        auto* navigateur = static_cast<Navigateur*>(user_data);
        navigateur->sauvegarderConfiguration();
        gtk_main_quit();  // Quitter proprement l'application
        return FALSE;  
    }), this);

    // Conteneur principal
    conteneurPrincipal = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(fenetre), conteneurPrincipal);

    // Ajout de la barre d'onglets
    initialiserBarreOnglets();
    ajouterNouvelOnglet(homepage);
    initialiserBarreNavigation();
    initialiserBarreFavoris();
    moteurRendu->initialiserRendu(conteneurPrincipal);

    moteurRendu->connecterSignalURLChangee([this](const std::string& url) {
        if (barreURL) {
            gtk_entry_set_text(GTK_ENTRY(barreURL), url.c_str());
        }
    });

    GtkWidget* boutonEtoile = moteurRendu->creerBouton("☆", G_CALLBACK(Navigateur::onBoutonFavorisClicked), this);
    gtk_box_pack_start(GTK_BOX(barreNavigation), boutonEtoile, FALSE, FALSE, 0);

    initialiserPopoverFavoris();
    gtk_widget_show_all(fenetre);
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
    barreNavigation = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);


    // Ajout des boutons de navigation et d'accès à la page d'accueil
    ajouterBouton(barreNavigation, "go-previous", G_CALLBACK(on_naviguer_retour), this);
    ajouterBouton(barreNavigation, "go-next", G_CALLBACK(on_naviguer_suivant), this);
    ajouterBouton(barreNavigation, "view-refresh", G_CALLBACK(on_rafraichir_page), this);
    ajouterBouton(barreNavigation, "go-home", G_CALLBACK(&Navigateur::onAllerAccueil), this);
    barreURL = moteurRendu->creerChampTexte(G_CALLBACK(&Navigateur::onBarreURLActivate), this);
    gtk_box_pack_start(GTK_BOX(barreNavigation), barreURL, TRUE, TRUE, 0);

    GtkWidget* boutonFavoris = moteurRendu->creerBouton("star", G_CALLBACK(Navigateur::onBoutonFavorisClicked), this);
    gtk_box_pack_start(GTK_BOX(barreNavigation), boutonFavoris, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(conteneurPrincipal), barreNavigation, FALSE, FALSE, 0);
}


std::string Navigateur::getURLActuelle() const {
    return moteurRendu->obtenirURLActuelle();
}


void Navigateur::initialiserBarreFavoris() {
    if (barreFavoris) {
        gtk_widget_destroy(barreFavoris); // Nettoyage de l'ancienne barre
    }

    barreFavoris = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    int largeurDispo = gtk_widget_get_allocated_width(conteneurPrincipal);
    int largeurActuelle = 0;
    bool boutonAjoute = false;

    for (const auto& favori : *favoris) {
        GtkWidget* boutonFavori = moteurRendu->creerBouton(favori["name"], nullptr, nullptr);
        auto* data = new std::pair<Navigateur*, std::string>(this, favori["url"]);
        g_signal_connect(boutonFavori, "clicked", G_CALLBACK(on_favori_clicked), data);

        int largeurBouton = 80; // Largeur estimée d'un bouton
        if (largeurActuelle + largeurBouton > largeurDispo) {
            // Ajouter le bouton ">>" pour les favoris restants
            if (!boutonAjoute) {
                GtkWidget* boutonPlus = moteurRendu->creerBouton(">>", G_CALLBACK(+[](GtkButton*, gpointer user_data) {
                    auto* navigateur = static_cast<Navigateur*>(user_data);
                    navigateur->afficherMenuFavorisRestants();
                }), this);
                gtk_box_pack_start(GTK_BOX(barreFavoris), boutonPlus, FALSE, FALSE, 5);
                boutonAjoute = true;
            }
        } else {
            gtk_box_pack_start(GTK_BOX(barreFavoris), boutonFavori, FALSE, FALSE, 5);
            largeurActuelle += largeurBouton;
        }
    }

    gtk_box_pack_start(GTK_BOX(conteneurPrincipal), barreFavoris, FALSE, FALSE, 5);
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
    } else {
        gtk_widget_show_all(gestionnaireFavoris->getFenetre()); 
    }
}


void Navigateur::rafraichirBarreFavoris() {
    if (GTK_IS_WIDGET(barreFavoris)) {
        gtk_widget_destroy(barreFavoris);
    }

    barreFavoris = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

    if (favoris.empty()) {
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

    gtk_box_pack_start(GTK_BOX(conteneurPrincipal), barreFavoris, FALSE, FALSE, 5);
    gtk_widget_show_all(barreFavoris);
}


void Navigateur::mettreAJourBoutonEtoile() {
    std::string urlActuelle = getURLActuelle();
    bool estDejaFavori = std::any_of(
        favoris.begin(), favoris.end(),
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
    gtk_widget_show_all(popoverFavoris);
}


void Navigateur::initialiserBarreOnglets() {
    barreOnglets = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

    // Bouton pour changer de groupe
    GtkWidget* boutonChangerGroupe = gtk_button_new_with_label("Changer Groupe");
    gtk_box_pack_start(GTK_BOX(barreOnglets), boutonChangerGroupe, FALSE, FALSE, 0);

    // Création du menu contextuel pour les groupes
    GtkWidget* menuGroupes = gtk_menu_new();

    // Ajouter un champ texte pour nommer le groupe
    GtkWidget* entryNouveauGroupeItem = gtk_menu_item_new();  // Créer un item vide
    GtkWidget* entryNouveauGroupe = gtk_entry_new();  // Utilisation correcte d'un GtkEntry pour la saisie
    gtk_entry_set_placeholder_text(GTK_ENTRY(entryNouveauGroupe), "Nouveau groupe...");

    gtk_container_add(GTK_CONTAINER(entryNouveauGroupeItem), entryNouveauGroupe);
    gtk_menu_shell_append(GTK_MENU_SHELL(menuGroupes), entryNouveauGroupeItem);
    gtk_widget_show_all(entryNouveauGroupeItem);

    // Ajouter un bouton pour créer un groupe
    GtkWidget* boutonAjouterGroupe = gtk_menu_item_new_with_label("Créer Groupe");
    
    auto* data = new std::pair<Navigateur*, GtkWidget*>(this, entryNouveauGroupe);
    
    g_signal_connect(boutonAjouterGroupe, "activate", G_CALLBACK(on_ajouter_groupe), data);
    gtk_menu_shell_append(GTK_MENU_SHELL(menuGroupes), boutonAjouterGroupe);

    // Lister les groupes existants dans le menu
    for (const auto& groupe : gestionnaireOnglets->getGroupes()) {
        GtkWidget* itemGroupe = gtk_menu_item_new_with_label(groupe.c_str());
        g_signal_connect(itemGroupe, "activate", G_CALLBACK(on_changer_groupe), this);
        gtk_menu_shell_append(GTK_MENU_SHELL(menuGroupes), itemGroupe);
    }

    // Associer le menu contextuel au bouton
    g_signal_connect(boutonChangerGroupe, "clicked", G_CALLBACK(+[](GtkWidget* widget, gpointer data) {
        gtk_menu_popup_at_widget(GTK_MENU(data), widget, GDK_GRAVITY_SOUTH, GDK_GRAVITY_NORTH, nullptr);
    }), menuGroupes);

    // Ajouter le bouton "+"
    GtkWidget* boutonAjouterOnglet = moteurRendu->creerBouton("list-add", G_CALLBACK(on_ajouter_onglet), this);


    gtk_box_pack_start(GTK_BOX(barreOnglets), boutonChangerGroupe, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(barreOnglets), boutonAjouterOnglet, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(conteneurPrincipal), barreOnglets, FALSE, FALSE, 0);
}

void Navigateur::changerGroupeOnglets(const std::string& nomGroupe) {
    gestionnaireOnglets->changerGroupeActif(nomGroupe);
    gtk_widget_destroy(barreOnglets);
    initialiserBarreOnglets();
}


void Navigateur::ajouterNouvelOnglet(const std::string &url) {
    gestionnaireOnglets->ajouterOnglet(gestionnaireOnglets->getGroupeActif(), url);

    GtkWidget *hboxOnglet = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    GtkWidget *boutonTitre = gtk_button_new_with_label("Nouvel Onglet");
    gtk_box_pack_start(GTK_BOX(hboxOnglet), boutonTitre, FALSE, FALSE, 0);


    GtkWidget *boutonFermer = moteurRendu->creerBouton("window-close", G_CALLBACK(+[](GtkButton *button, Navigateur *n) {
        GtkWidget *parent = gtk_widget_get_parent(GTK_WIDGET(button));
        n->supprimerOnglet(parent);
    }), this);

    // Correct : lambda avec capture explicite de `this`
    g_signal_connect(hboxOnglet, "button-press-event", G_CALLBACK(+[](GtkWidget* widget, GdkEventButton*, gpointer user_data) {
        auto* n = static_cast<Navigateur*>(user_data);
        if (n) {
            n->changerOngletActif(widget);
        }
        return TRUE; // Pour capturer l'évènement correctemnt on le passe à TRUE
    }), this);


    gtk_box_pack_start(GTK_BOX(hboxOnglet), boutonFermer, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(barreOnglets), hboxOnglet, FALSE, FALSE, 0);
    onglets.push_back({url, hboxOnglet});
    moteurRendu->afficherPage(url);
    GtkWidget *labelTitre = gtk_label_new("Nouvel Onglet");  // Ajouté juste avant la capture
    moteurRendu->connecterSignalPageChargee([=](const std::string &titre) {
        gtk_label_set_text(GTK_LABEL(labelTitre), titre.c_str());
    });


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
    nlohmann::json favoris = GestionnaireFichiers::lireJSON(cheminFavoris);

    if (favoris.is_null() || favoris.empty()) {
        std::cerr << "Aucun favori trouvé, initialisation avec un favori par défaut." << std::endl;
        favoris = nlohmann::json::array({
            {{"name", "DuckDuckGo"}, {"url", "https://www.duckduckgo.com"}, {"tag", "Recherche"}}
        });
        GestionnaireFichiers::ecrireJSON(cheminFavoris, favoris);
    }
    
    if (config.is_null() || config.empty()) {
        std::cerr << "Fichier de configuration non trouvé ou vide. Création d'une configuration par défaut." << std::endl;
        config["homepage"] = "https://www.duckduckgo.com";
        GestionnaireFichiers::ecrireJSON(chemin, config);
    }

    if (config.contains("onglets")) {
        for (const auto& onglet : config["onglets"]) {
            ajouterNouvelOnglet(onglet["url"], onglet["etat"]);
        }
    }
    
    homepage = config.value("homepage", "https://www.duckduckgo.com");
    std::cout << "Page d'accueil définie sur : " << homepage << std::endl;
}


void Navigateur::sauvegarderConfiguration() {
    nlohmann::json config;
    config["homepage"] = homepage;
    GestionnaireFichiers::ecrireJSON(GestionnaireFichiers::cheminConfigJSON(), config);
    
    GestionnaireFichiers::ecrireJSON(GestionnaireFichiers::cheminFavorisJSON(), favoris);
    std::cout << "Favoris sauvegardés automatiquement !" << std::endl;
    
    nlohmann::json ongletsJson = nlohmann::json::array();
    for (const auto& [url, widget] : onglets) {
        ongletsJson.push_back({{"url", url}, {"etat", widget->etat}});
    }
    config["onglets"] = ongletsJson;
}

void Navigateur::ajouterFavori(const std::string& nom, const std::string& url, const std::string& tag) {
    gestionnaireFavoris->ajouterFavori(nom, url, tag);
    rafraichirBarreFavoris();
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

    std::string cheminCSS = GestionnaireFichiers::cheminStylesCSS();

    if (error) {
        g_warning("Erreur de chargement du CSS : %s", error->message);
        g_error_free(error);
    }
    gtk_widget_set_name(barreFavoris, "barre-favoris");

    gtk_css_provider_load_from_data(provider, 
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
    bool estDejaFavori = std::any_of(
        navigateur->getFavoris().begin(),
        navigateur->getFavoris().end(),
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
        gtk_popover_popup(GTK_POPOVER(navigateur->popoverFavoris));
    }
}

void Navigateur::on_supprimer_favori(GtkWidget* widget, gpointer user_data) {
    auto* data = static_cast<std::pair<Navigateur*, GtkWidget*>*>(user_data);
    if (data && data->first) {
        data->first->supprimerFavori(data->second);
    }
    delete data;
}

void Navigateur::afficherPaletteCommandes() {
    if (commandPalette && fenetre) {
        commandPalette->setCurrentWebView(moteurRendu.get());
        commandPalette->showPalette(GTK_WINDOW(fenetre));
    }
}

void Navigateur::hibernerOnglet(const std::string& url) {
    for (auto& onglet : onglets) {
        if (onglet.first == url) {
            onglet.second->etat = "hiberné";
            gestionnaireMemoire->hibernerOnglet(onglet.first);
            std::cout << "Onglet mis en veille : " << url << std::endl;
            return;
        }
    }
    std::cerr << "Onglet introuvable pour mise en veille : " << url << std::endl;
}
void Navigateur::reactiverOnglet(const std::string& url) {
    for (auto& onglet : onglets) {
        if (onglet.first == url && onglet.second->etat == "hiberné") {
            onglet.second->etat = "actif";
            moteurRendu->afficherPage(url);
            std::cout << "Onglet réactivé : " << url << std::endl;
            return;
        }
    }
    std::cerr << "Onglet introuvable pour réactivation : " << url << std::endl;
}
