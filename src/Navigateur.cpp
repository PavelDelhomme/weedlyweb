#include "Navigateur.h"
#include "GestionnaireFichiers.h"
#include "GestionnaireFavoris.h"
#include <iostream>
#include <filesystem>
#include <string>
#include <fstream>
#include <set>

std::string obtenirCheminAbsolu(const std::string& fichier) {
    return std::filesystem::current_path().string() + "/" + fichier;
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
    }
    delete data;  // Libérer la mémoire allouée
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
        if ((key_event->state & GDK_CONTROL_MASK) && key_event->keyval == GDK_KEY_t) {
            navigateur->ajouterNouvelOnglet(navigateur->getHomepage());
        }
    }
    return FALSE;
}

static gboolean on_favoris_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    if (event->type == GDK_BUTTON_PRESS && event->button == 3) { // Bouton droit
        auto *navigateur = static_cast<Navigateur *>(user_data);
        navigateur->creerMenuContextuel(widget);
        return TRUE;
    }
    return FALSE;
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
      moteurScript(std::make_unique<MoteurScript>())
{
    gestionnaireFavoris = std::make_unique<GestionnaireFavoris>(favoris, [this]() { rafraichirBarreFavoris(); });
    chargerConfiguration();
    construireInterface();
}


Navigateur::~Navigateur() {
    if (fenetre) gtk_widget_destroy(fenetre);
}

nlohmann::json& Navigateur::getFavoris() {
    return favoris;
}

void Navigateur::construireInterface() {
    fenetre = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(fenetre), "WeedlyWeb");
    gtk_window_set_default_size(GTK_WINDOW(fenetre), 1024, 768);

    // COnnexion sécurisée du signal de fermeture
    g_signal_connect(fenetre, "delete-event", G_CALLBACK(on_delete_event), this);

    // Conteneur principal
    conteneurPrincipal = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(fenetre), conteneurPrincipal);

    // Ajout de la barre d'onglets
    initialiserBarreOnglets();
    chargerURL(homepage);
    ajouterNouvelOnglet(homepage);
    initialiserBarreNavigation();
    initialiserBarreFavoris();
    moteurRendu->initialiserRendu(conteneurPrincipal);

    moteurRendu->connecterSignalURLChangee([this](const std::string& url) {
        if (barreURL) {
            gtk_entry_set_text(GTK_ENTRY(barreURL), url.c_str());
        }
    });

    // Ajout de la sauvegarde automatique lors de la fermeture de la fenêtre principale
    g_signal_connect(fenetre, "delete-event", G_CALLBACK(+[](GtkWidget*, GdkEvent*, gpointer user_data) -> gboolean {
        auto* navigateur = static_cast<Navigateur*>(user_data);
        navigateur->sauvegarderConfiguration();
        gtk_main_quit();  // Quitter proprement l'application
        return FALSE;  // Laisse GTK continuer la fermeture
    }), this);

    initialiserPopoverFavoris();
    gtk_widget_show_all(fenetre);
}
void Navigateur::ajouterBouton(GtkWidget* conteneur, const std::string& iconName, GCallback callback, gpointer data) {
    GtkWidget *bouton = moteurRendu->creerBouton(iconName, callback, data);
    gtk_box_pack_start(GTK_BOX(conteneur), bouton, FALSE, FALSE, 0);
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

    // ajouterBouton(barreNavigation, "go-previous", G_CALLBACK(&Navigateur::onNaviguerRetourWrapper), this);
    // ajouterBouton(barreNavigation, "go-next", G_CALLBACK(&Navigateur::onNaviguerSuivantWrapper), this);
    // ajouterBouton(barreNavigation, "view-refresh", G_CALLBACK(&Navigateur::onRafraichirPageWrapper), this);
    // ajouterBouton(barreNavigation, "go-previous", G_CALLBACK(on_naviguer_retour), this);
    // ajouterBouton(barreNavigation, "go-next", G_CALLBACK(on_naviguer_suivant), this);
    // ajouterBouton(barreNavigation, "view-refresh", G_CALLBACK(on_rafraichir_page), this);
    // Ajout des boutons de navigation et d'accès à la page d'accueil
    ajouterBouton(barreNavigation, "go-previous", G_CALLBACK(on_naviguer_retour), this);
    ajouterBouton(barreNavigation, "go-next", G_CALLBACK(on_naviguer_suivant), this);
    ajouterBouton(barreNavigation, "view-refresh", G_CALLBACK(on_rafraichir_page), this);
    ajouterBouton(barreNavigation, "go-home", G_CALLBACK(&Navigateur::onAllerAccueil), this);

    // ajouterBouton(barreNavigation, "go-home", G_CALLBACK(&Navigateur::onAllerAccueil), this);

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
        gtk_widget_destroy(barreFavoris);
    }

    barreFavoris = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

    // Toujours afficher la barre, même si elle est vide
    if (favoris.empty()) {
        GtkWidget *labelAucunFavori = gtk_label_new("Aucun favori");
        gtk_box_pack_start(GTK_BOX(barreFavoris), labelAucunFavori, FALSE, FALSE, 5);
    }
    for (const auto& favori : favoris) {
        if (favori.contains("name") && favori.contains("url")) {
            GtkWidget *boutonFavori = moteurRendu->creerBouton(favori["name"], nullptr, nullptr);

            std::string urlCopie = favori["url"];
            std::pair<Navigateur*, std::string>* data = new std::pair<Navigateur*, std::string>(this, urlCopie);
            g_signal_connect(boutonFavori, "clicked", G_CALLBACK(on_favori_clicked), data);
            gtk_box_pack_start(GTK_BOX(barreFavoris), boutonFavori, FALSE, FALSE, 5);
        }
    }

    if (gtk_widget_get_parent(barreFavoris)) {
        gtk_container_remove(GTK_CONTAINER(gtk_widget_get_parent(barreFavoris)), barreFavoris);
    }
    g_signal_connect(barreFavoris, "button-press-event", G_CALLBACK(on_favoris_button_press), this);

    gtk_box_pack_start(GTK_BOX(conteneurPrincipal), barreFavoris, FALSE, FALSE, 5);
    //gtk_widget_show_all(conteneurPrincipal);
    gtk_widget_show_all(barreFavoris);
}

void Navigateur::afficherGestionnaireFavoris() {
    if (!gestionnaireFavoris) {
        gestionnaireFavoris = std::make_unique<GestionnaireFavoris>(favoris, [this]() { rafraichirBarreFavoris(); });
    }
}

void Navigateur::rafraichirBarreFavoris() {
    gtk_widget_destroy(barreFavoris);
    initialiserBarreFavoris();
}

void Navigateur::initialiserPopoverFavoris() {
    popoverFavoris = gtk_popover_new(barreFavoris); // Attaché à la barre de favoris
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

}


void Navigateur::initialiserBarreOnglets() {
    barreOnglets = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

    // Bouton pour changer de groupe
    GtkWidget* boutonChangerGroupe = gtk_button_new_with_label("Changer Groupe");
    gtk_box_pack_start(GTK_BOX(barreOnglets), boutonChangerGroupe, FALSE, FALSE, 0);

    // Création du menu contextuel pour les groupes
    GtkWidget* menuGroupes = gtk_menu_new();

    // Ajouter un champ texte pour nommer le groupe
    GtkWidget* entryNouveauGroupe = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entryNouveauGroupe), "Nouveau groupe...");
    gtk_menu_shell_append(GTK_MENU_SHELL(menuGroupes), entryNouveauGroupe);

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
    // GtkWidget *boutonAjouterOnglet = moteurRendu->creerBouton("list-add", G_CALLBACK(+[](GtkButton *, Navigateur *n) {
    //     n->ajouterNouvelOnglet(n->homepage);
    // }), this);
    GtkWidget* boutonAjouterOnglet = moteurRendu->creerBouton("list-add", G_CALLBACK(on_ajouter_onglet), this);


    gtk_box_pack_start(GTK_BOX(barreOnglets), boutonChangerGroupe, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(barreOnglets), boutonAjouterOnglet, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(conteneurPrincipal), barreOnglets, FALSE, FALSE, 0);
}

void Navigateur::changerGroupeOnglets(const std::string& nomGroupe) {
    gestionnaireOnglets->changerGroupeActif(nomGroupe);
    gtk_widget_destroy(barreOnglets);
    initialiserBarreOnglets();
    
    // Charger les onglets du groupe actif
    for (const auto& url : gestionnaireOnglets->getOngletsDuGroupe(nomGroupe)) {
        ajouterNouvelOnglet(url);
    }
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

void Navigateur::supprimerOnglet(GtkWidget *ongletWidget) {
    // Trouver l'onglet à supprimer
    auto it = std::find_if(onglets.begin(), onglets.end(), [ongletWidget](const auto &pair) {
        return pair.second == ongletWidget;
    });

    if (it != onglets.end()) {
        GtkWidget *hboxOnglet = it->second;

        // Conversion explicite vers WebKitWebView*
        WebKitWebView *vueWeb = WEBKIT_WEB_VIEW(gtk_bin_get_child(GTK_BIN(hboxOnglet)));
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
    moteurRendu->connecterSignalPageChargee([this](const std::string &titre) {
        if (!onglets.empty()) {
            GtkWidget *hboxOnglet = onglets.back().second;
            GtkWidget* labelTitre = obtenirPremierEnfant(hboxOnglet);
            if (GTK_IS_LABEL(labelTitre)) {
                gtk_label_set_text(GTK_LABEL(labelTitre), titre.c_str());
            } else {
                std::cerr << "Erreur : widget attendu est un GtkLabel, mais ce n'est pas le cas." << std::endl;
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
    
    homepage = config.value("homepage", "https://www.duckduckgo.com");
    std::cout << "Page d'accueil définie sur : " << homepage << std::endl;
}


void Navigateur::sauvegarderConfiguration() {
    nlohmann::json config;
    config["homepage"] = homepage;
    GestionnaireFichiers::ecrireJSON(GestionnaireFichiers::cheminConfigJSON(), config);
    
    std::string cheminFavoris = GestionnaireFichiers::cheminFavorisJSON();
    GestionnaireFichiers::ecrireJSON(cheminFavoris, favoris);
    std::cout << "Favoris sauvegardés automatiquement !" << std::endl;
    
}


void Navigateur::ajouterFavori(const std::string& nom, const std::string& url, const std::string& tag) {
    // Vérifie si l'URL existe déjà
    for (const auto& favori : favoris) {
        if (favori["url"] == url) {
            std::cerr << "Favori déjà existant : " << url << std::endl;
            return;
        }
    }

    // Ajoute le nouveau favori
    favoris.push_back({{"name", nom}, {"url", url}, {"tag", tag}});
    gestionnaireFavoris->sauvegarderModifications();
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


// void Navigateur::onBarreURLActivate(GtkEntry *entry, Navigateur *n) {
//     n->chargerURL(gtk_entry_get_text(entry));
// }

void Navigateur::onBarreURLActivate(GtkEntry* entry, gpointer user_data) {
    auto* navigateur = static_cast<Navigateur*>(user_data);
    std::string url = gtk_entry_get_text(entry);
    navigateur->chargerURL(url);
}

void Navigateur::fermerApplication() {
    sauvegarderConfiguration();
    if (fenetre) gtk_widget_destroy(fenetre);
    gtk_main_quit();
}

void Navigateur::onNaviguerRetourWrapper(GtkButton* button, gpointer user_data) {
    auto* navigateur = static_cast<Navigateur*>(user_data);
    navigateur->onNaviguerRetour(button, navigateur);
}



void Navigateur::onNaviguerSuivantWrapper(GtkButton *button, gpointer user_data) {
    // auto *navigateur = static_cast<Navigateur*>(user_data);
    // if (navigateur) {
    //     navigateur->onNaviguerSuivant(button, navigateur);
    // }
    static_cast<Navigateur*>(user_data)->onNaviguerSuivant(button, static_cast<Navigateur*>(user_data));
}

void Navigateur::onRafraichirPageWrapper(GtkButton *button, gpointer user_data) {
    // auto *navigateur = static_cast<Navigateur*>(user_data);
    // if (navigateur) {
    //     navigateur->onRafraichirPage(button, navigateur);
    // }
    static_cast<Navigateur*>(user_data)->onRafraichirPage(button, static_cast<Navigateur*>(user_data));
}

// void Navigateur::creerMenuContextuel(GtkWidget* bouton) {
//     GtkWidget *menu = gtk_menu_new();

//     GtkWidget *ajouterItem = gtk_menu_item_new_with_label("Ajouter Favori");
//     // g_signal_connect(ajouterItem, "activate", G_CALLBACK(+[](GtkWidget*, gpointer user_data) {
//     //     auto* navigateur = static_cast<Navigateur*>(user_data);
//     //     std::string urlActuelle = navigateur->getURLActuelle();
//     //     navigateur->ajouterFavori("Nouveau Favori", urlActuelle, "Général");
//     // }), this);
//     g_signal_connect(ajouterItem, "activate", G_CALLBACK(on_ajouter_favori_menu), this);
//     gtk_menu_shell_append(GTK_MENU_SHELL(menu), ajouterItem);

//     // // Création des champs de saisie pour URL et nom
//     // GtkWidget *entryNom = gtk_entry_new();
//     // gtk_entry_set_placeholder_text(GTK_ENTRY(entryNom), "Nom du favori");
//     // gtk_menu_shell_append(GTK_MENU_SHELL(menu), entryNom);

//     // GtkWidget *entryURL = gtk_entry_new();
//     // gtk_entry_set_placeholder_text(GTK_ENTRY(entryURL), "URL");
//     // gtk_menu_shell_append(GTK_MENU_SHELL(menu), entryURL);

//     // // Correction ici : changement de nom de la variable
//     // GtkWidget *ajouterItem2 = gtk_menu_item_new_with_label("Confirmer l'ajout");
//     // auto* data = new std::pair<Navigateur*, std::pair<GtkWidget*, GtkWidget*>>(this, {entryNom, entryURL});
//     // g_signal_connect(ajouterItem2, "activate", G_CALLBACK(on_ajouter_favori_menu), data);

//     // gtk_menu_shell_append(GTK_MENU_SHELL(menu), ajouterItem2);



//     gtk_widget_show_all(menu);
//     gtk_menu_popup_at_widget(GTK_MENU(menu), bouton, GDK_GRAVITY_SOUTH, GDK_GRAVITY_NORTH, nullptr);
// }

// void Navigateur::creerMenuContextuel(GtkWidget* bouton) {
//     GtkWidget *menu = gtk_menu_new();

//     // Créer un item pour ajouter un favori
//     GtkWidget *ajouterItem = gtk_menu_item_new_with_label("Ajouter Favori");
//     g_signal_connect(ajouterItem, "activate", G_CALLBACK(on_ajouter_favori_menu), this);
//     gtk_menu_shell_append(GTK_MENU_SHELL(menu), ajouterItem);

//     // Créer un item pour gérer les favoris
//     GtkWidget *gererFavorisItem = gtk_menu_item_new_with_label("Gérer les Favoris");
//     g_signal_connect(gererFavorisItem, "activate", G_CALLBACK(+[](GtkWidget*, gpointer user_data) {
//         auto* navigateur = static_cast<Navigateur*>(user_data);
//         navigateur->afficherGestionnaireFavoris();
//     }), this);
//     gtk_menu_shell_append(GTK_MENU_SHELL(menu), gererFavorisItem);

//     // Afficher le menu correctement
//     gtk_widget_show_all(menu);
//     gtk_menu_popup_at_widget(GTK_MENU(menu), bouton, GDK_GRAVITY_SOUTH, GDK_GRAVITY_NORTH, nullptr);
// }
void Navigateur::creerMenuContextuel(GtkWidget* bouton) {
    GtkWidget *menu = gtk_menu_new();

    GtkWidget *ajouterItem = gtk_menu_item_new_with_label("Ajouter Favori");
    g_signal_connect(ajouterItem, "activate", G_CALLBACK(on_ajouter_favori_menu), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), ajouterItem);

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
        [&urlActuelle](const nlohmann::json& favori) {
            return favori.contains("url") && favori["url"] == urlActuelle;
        }
    );

    // Si l'URL est déjà un favori, afficher un message et ne pas ouvrir le popover
    if (estDejaFavori) {
        std::cerr << "URL déjà ajoutée aux favoris : " << urlActuelle << std::endl;
        gtk_button_set_label(button, "*"); // Marquer l'URL comme déjà en favoris
    } else {
        // Sinon, ouvrir le popover pour permettre l'ajout
        gtk_popover_popup(GTK_POPOVER(navigateur->popoverFavoris));
    }
}
