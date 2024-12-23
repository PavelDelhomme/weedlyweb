#include "Navigateur.h"
#include "GestionnaireFichiers.h"
#include <iostream>
#include <filesystem>
#include <string>
#include <fstream>

std::string obtenirCheminAbsolu(const std::string& fichier) {
    return std::filesystem::current_path().string() + "/" + fichier;
}

Navigateur::Navigateur() 
    : moteurRendu(std::make_unique<MoteurRendu>()),
      gestionnaireHTTP(std::make_unique<GestionnaireHTTP>()),
      gestionnaireMemoire(std::make_unique<GestionnaireMemoire>()) {
    chargerConfiguration();
    chargerFavoris();
    construireInterface();
}


Navigateur::~Navigateur() {
    /*if (fenetre) {
        gtk_widget_destroy(fenetre);
        fenetre = nullptr;
    }
    if (barreURL) {
        gtk_widget_destroy(barreURL);
        barreURL = nullptr;
    }
    if (boutonAccueil) {
        gtk_widget_destroy(boutonAccueil);
        boutonAccueil = nullptr;
    }*/
    if (fenetre) gtk_widget_destroy(fenetre);    
}

void Navigateur::construireInterface() {
    fenetre = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(fenetre), "WeedlyWeb");
    gtk_window_set_default_size(GTK_WINDOW(fenetre), 1024, 768);

    // Conteneur principal
    conteneurPrincipal = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(fenetre), conteneurPrincipal);

    // Ajout de la barre d'onglets
    initialiserBarreOnglets();
    chargerURL(homepage);
    initialiserBarreNavigation();
    initialiserBarreFavoris();
    moteurRendu->initialiserRendu(conteneurPrincipal);

    moteurRendu->connecterSignalURLChangee([this](const std::string& url) {
        if (barreURL) {
            gtk_entry_set_text(GTK_ENTRY(barreURL), url.c_str());
        }
    });

    g_signal_connect(fenetre, "destroy", G_CALLBACK(+[](GtkWidget *, gpointer data) {
        static_cast<Navigateur *>(data)->fermerApplication();
    }), this);

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

    ajouterBouton(barreNavigation, "go-previous", G_CALLBACK(&Navigateur::onNaviguerRetour), this);
    ajouterBouton(barreNavigation, "go-next", G_CALLBACK(&Navigateur::onNaviguerSuivant), this);
    ajouterBouton(barreNavigation, "view-refresh", G_CALLBACK(&Navigateur::onRafraichirPage), this);
    ajouterBouton(barreNavigation, "go-home", G_CALLBACK(&Navigateur::onAllerAccueil), this);


    barreURL = moteurRendu->creerChampTexte(G_CALLBACK(&Navigateur::onBarreURLActivate), this);
    gtk_box_pack_start(GTK_BOX(barreNavigation), barreURL, TRUE, TRUE, 0);

    GtkWidget *boutonFavoris = moteurRendu->creerBouton("star", G_CALLBACK(+[](GtkButton *, Navigateur *n) {
        std::string url = n->moteurRendu->obtenirURLActuelle();
        if (!url.empty()) {
            n->ajouterFavori("Favori", url, ""); // Ajoute le favori avec un nom par défaut
            n->rafraichirBarreFavoris(); // Met à jour la barre des favoris
        }
    }), this); // Ferme correctement la lambda ici

    gtk_box_pack_start(GTK_BOX(barreNavigation), boutonFavoris, FALSE, FALSE, 0); // Ajoute boutonFavoris à la barre
        
    GtkWidget *boutonParametres = moteurRendu->creerBouton("preferences-system", G_CALLBACK(+[](GtkButton *, Navigateur *n) {
        n->afficherParametres();
    }), this);
    gtk_box_pack_start(GTK_BOX(barreNavigation), boutonParametres, FALSE, FALSE, 0); // Ajoute boutonParametres à la barre

    gtk_box_pack_start(GTK_BOX(conteneurPrincipal), barreNavigation, FALSE, FALSE, 0);
}

void Navigateur::initialiserBarreFavoris() {
    barreFavoris = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

    if (favoris.empty()) {
        GtkWidget *labelVide = gtk_label_new("Aucun favori disponible.");
        gtk_box_pack_start(GTK_BOX(barreFavoris), labelVide, FALSE, FALSE, 0);
    } else {
        for (const auto &favori : favoris) {
            std::string nom = favori["name"];
            std::string url = favori["url"];
            GtkWidget *boutonFavori = moteurRendu->creerBouton(
                nom,
                G_CALLBACK(Navigateur::onCliqueFavoriWrapper),
                new std::pair<Navigateur*, std::string>(this, url)
            );
            gtk_box_pack_start(GTK_BOX(barreFavoris), boutonFavori, FALSE, FALSE, 0);
        }
    }

    gtk_box_pack_start(GTK_BOX(conteneurPrincipal), barreFavoris, FALSE, FALSE, 0);
    gtk_widget_show_all(conteneurPrincipal);
}

void Navigateur::onCliqueFavoriWrapper(GtkButton *button, gpointer user_data) {
    auto *data = static_cast<std::pair<Navigateur*, std::string>*>(user_data);
    if (data) {
        data->first->chargerURL(data->second);
        delete data; // Libérez la mémoire
    }
}


void Navigateur::initialiserBarreOnglets() {
    barreOnglets = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    ajouterNouvelOnglet(homepage);

    // Ajouter le bouton "+"
    GtkWidget *boutonAjouterOnglet = moteurRendu->creerBouton("list-add", G_CALLBACK(+[](GtkButton *, Navigateur *n) {
        n->ajouterNouvelOnglet(n->homepage);
    }), this);

    gtk_box_pack_start(GTK_BOX(barreOnglets), boutonAjouterOnglet, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(conteneurPrincipal), barreOnglets, FALSE, FALSE, 0);
}

void Navigateur::ajouterNouvelOnglet(const std::string &url) {
    GtkWidget *hboxOnglet = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

    // Titre de l'onglet
    GtkWidget *labelTitre = gtk_label_new("Chargement...");
    gtk_box_pack_start(GTK_BOX(hboxOnglet), labelTitre, FALSE, FALSE, 0);

    // Bouton "Fermer"
    GtkWidget *boutonFermer = moteurRendu->creerBouton("window-close", G_CALLBACK(+[](GtkButton *button, Navigateur *n) {
        GtkWidget *parent = gtk_widget_get_parent(GTK_WIDGET(button));
        n->supprimerOnglet(parent);
    }), this);
    gtk_widget_set_visible(boutonFermer, FALSE); // Masquer le bouton au départ
    gtk_box_pack_start(GTK_BOX(hboxOnglet), boutonFermer, FALSE, FALSE, 0);

    // Connecter les signaux pour afficher/masquer la croix au survol
    g_signal_connect(hboxOnglet, "enter-notify-event", G_CALLBACK(+[](GtkWidget *widget, GdkEventCrossing *, GtkWidget *bouton) {
        gtk_widget_set_visible(bouton, TRUE); // Afficher la croix
        return FALSE;
    }), boutonFermer);
    g_signal_connect(hboxOnglet, "leave-notify-event", G_CALLBACK(+[](GtkWidget *widget, GdkEventCrossing *, GtkWidget *bouton) {
        gtk_widget_set_visible(bouton, FALSE); // Masquer la croix
        return FALSE;
    }), boutonFermer);

    onglets.push_back({url, hboxOnglet});
    gtk_box_pack_start(GTK_BOX(barreOnglets), hboxOnglet, FALSE, FALSE, 0);

    // Déplacer le bouton "+" à la fin
    GtkWidget *boutonAjouterOnglet = obtenirDernierEnfant(GTK_WIDGET(barreOnglets));
    gtk_box_reorder_child(GTK_BOX(barreOnglets), boutonAjouterOnglet, -1);

    gtk_widget_show_all(barreOnglets);

    if (!url.empty()) {
        chargerURL(url);
        // Mettre à jour le titre de l'onglet lorsqu'il est chargé
        moteurRendu->connecterSignalPageChargee([labelTitre](const std::string &titre) {
            gtk_label_set_text(GTK_LABEL(labelTitre), titre.c_str());
        });
    }
}


/*
void Navigateur::ajouterNouvelOnglet(const std::string &url) {
    GtkWidget *hboxOnglet = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);                         

    GtkWidget *labelTitre = gtk_label_new("Nouvel onglet");
    gtk_box_pack_start(GTK_BOX(hboxOnglet), labelTitre, FALSE, FALSE, 0);

    GtkWidget *boutonOnglet = moteurRendu->creerBouton("Nouvel onglet", G_CALLBACK(+[](GtkButton *, Navigateur *n) {
        n->chargerURL(n->homepage);
    }), this);
    gtk_box_pack_start(GTK_BOX(hboxOnglet), boutonOnglet, FALSE, FALSE, 0);

    // Bouton "Fermer"
    GtkWidget *boutonFermer = moteurRendu->creerBouton("X", G_CALLBACK(+[](GtkButton *button, Navigateur *n) {
        GtkWidget *parent = gtk_widget_get_parent(GTK_WIDGET(button)); // Trouver le parent
        n->supprimerOnglet(parent); // Supprimer l'onglet
    }), this);
    gtk_box_pack_start(GTK_BOX(hboxOnglet), boutonFermer, FALSE, FALSE, 0);

    // Ajouter à la liste des onglets
    onglets.push_back({url, hboxOnglet});
    gtk_box_pack_start(GTK_BOX(barreOnglets), hboxOnglet, FALSE, FALSE, 0);

    gtk_widget_show_all(barreOnglets);
}
*/
void Navigateur::supprimerOnglet(GtkWidget *ongletWidget) {
    // Trouver l'onglet à supprimer
    auto it = std::find_if(onglets.begin(), onglets.end(), [ongletWidget](const auto &pair) {
        return pair.second == ongletWidget;
    });

    if (it != onglets.end()) {
        // Supprimer le widget graphique de la barre d'onglets
        gtk_widget_destroy(it->second);
        // Supprimer l'onglet de la liste
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
            GtkWidget *labelTitre = obtenirPremierEnfant(GTK_WIDGET(hboxOnglet));
            gtk_label_set_text(GTK_LABEL(labelTitre), titre.c_str());
        }
    });
}

void Navigateur::afficherMessage(const std::string& message) {
    std::cout << "Message : " << message << std::endl;
}

/*
void Navigateur::chargerConfiguration() {
    try {
        std::ifstream fichier("config.json");
        if (!fichier.is_open()) {
            std::cerr << "Fichier de configuration non trouvé. Création d'une configuration par défaut." << std::endl;
            nlohmann::json config;
            config["homepage"] = "https://www.duckduckgo.com";
            std::ofstream fichierParDefaut("config.json");
            fichierParDefaut << config.dump(4);
            homepage = "https://www.duckduckgo.com";
        } else {
            nlohmann::json config;
            fichier >> config;
            homepage = config.value("homepage", "https://www.duckduckgo.com");
        }
    } catch (const std::exception& e) {
        std::cerr << "Erreur lors du chargement de la configuration : " << e.what() << std::endl;
        homepage = "https://www.duckduckgo.com";
    }
}*/
void Navigateur::chargerConfiguration() {
    std::string chemin = GestionnaireFichiers::cheminConfigJSON();
    nlohmann::json config = GestionnaireFichiers::lireJSON(chemin);
    
    // Si "homepage" n'est pas défini, crée un fichier de configuration par défaut
    if (config.is_null() || config.empty()) {
        std::cerr << "Fichier de configuration non trouvé ou vide. Création d'une configuration par défaut." << std::endl;
        config["homepage"] = "https://www.duckduckgo.com";
        GestionnaireFichiers::ecrireJSON(chemin, config);
    }
    
    homepage = config.value("homepage", "https://www.duckduckgo.com");
}

/*
void Navigateur::sauvegarderConfiguration() {
    nlohmann::json config;
    config["homepage"] = homepage;
    std::ofstream fichier("config.json");
    fichier << config.dump(4);
}*/
void Navigateur::sauvegarderConfiguration() {
    nlohmann::json config;
    config["homepage"] = homepage;
    GestionnaireFichiers::ecrireJSON(GestionnaireFichiers::cheminConfigJSON(), config);
}

/*
void Navigateur::chargerFavoris() {
    try {
        std::ifstream fichier("favoris.json");
        if (fichier.is_open()) {
            fichier >> favoris;
            if (!favoris.is_array()) { // Vérifie si favoris est bien un tableau JSON
                std::cerr << "Le contenu de favoris.json n'est pas un tableau valide. Réinitialisation." << std::endl;
                favoris = nlohmann::json::array();
            }
        } else {
            std::cerr << "Fichier favoris.json introuvable. Initialisation des favoris à un tableau vide." << std::endl;
            favoris = nlohmann::json::array();
        }
    } catch (const std::exception& e) {
        std::cerr << "Erreur lors du chargement des favoris : " << e.what() << std::endl;
        favoris = nlohmann::json::array();
    }
}*/
void Navigateur::chargerFavoris() {
    std::string chemin = GestionnaireFichiers::cheminFavorisJSON();
    favoris = GestionnaireFichiers::lireJSON(chemin);
    
    // Si le fichier est vide ou n'est pas un tableau JSON, créez un tableau vide
    if (favoris.is_null() || !favoris.is_array()) {
        std::cerr << "Fichier favoris.json introuvable ou invalide. Création d'un fichier vide." << std::endl;
        favoris = nlohmann::json::array();
        GestionnaireFichiers::ecrireJSON(chemin, favoris); // Sauvegarde du fichier vide
    }
}

/*
void Navigateur::sauvegarderFavoris() {
    try {
        std::ofstream fichier("favoris.json");
        if (!fichier.is_open()) {
            throw std::ios_base::failure("Impossible d'ouvrir favoris.json pour écriture");
        }
        fichier << favoris.dump(4); // Écrit le JSON (4 espaces d'indentation)
    } catch (const std::exception& e) {
        std::cerr << "Erreur lors de la sauvegarde des favoris : " << e.what() << std::endl;
    }
}
*/
void Navigateur::sauvegarderFavoris() {
    GestionnaireFichiers::ecrireJSON(GestionnaireFichiers::cheminFavorisJSON(), favoris);
}


void Navigateur::ajouterFavori(const std::string& nom, const std::string& url, const std::string& tag) {
    favoris.push_back({{"name", nom}, {"url", url}, {"tag", tag}});
    sauvegarderFavoris();
}

void Navigateur::rafraichirBarreFavoris() {
    gtk_widget_destroy(barreFavoris); // Supprime la barre actuelle
    initialiserBarreFavoris();        // Reconstruit la barre
    gtk_widget_show_all(conteneurPrincipal); // Met à jour l'interface
}


/*
void Navigateur::afficherParametres() {
    // Logique pour afficher une page de configuration
    std::string cheminParametres = obtenirCheminAbsolu("parametres.html");
    moteurRendu->afficherPage("file://" + cheminParametres);
}*/
void Navigateur::afficherParametres() {
    std::string cheminParametres = GestionnaireFichiers::cheminParametresHTML();
    moteurRendu->afficherPage("file://" + cheminParametres);
}




void Navigateur::configurerRaccourcisClavier() {
    g_signal_connect(fenetre, "key-press-event", G_CALLBACK(+[](GtkWidget *, GdkEventKey *event, Navigateur *n) {
        if ((event->state & GDK_CONTROL_MASK) && event->keyval == GDK_KEY_t) {
            n->ajouterNouvelOnglet();
        } else if ((event->state & GDK_CONTROL_MASK) && event->keyval == GDK_KEY_w) {
            if (!n->onglets.empty()) {
                n->supprimerOnglet(n->onglets.back().second);
            }
        } else if ((event->state & GDK_CONTROL_MASK) && event->keyval == GDK_KEY_d) {
            std::string url = n->moteurRendu->obtenirURLActuelle();
            if (!url.empty()) {
                n->ajouterFavori("Favori", url, "");
                n->rafraichirBarreFavoris();
            }
        }
        return FALSE;
    }), this);
}


/*
void Navigateur::chargerStyles() {
    GtkCssProvider *provider = gtk_css_provider_new();
    GError *error = NULL;

    std::string cheminCSS = obtenirCheminAbsolu("assets/styles/style.css");
    gtk_css_provider_load_from_path(provider, cheminCSS.c_str(), &error);

    if (error) {
        g_warning("Erreur de chargement du CSS : %s", error->message);
        g_error_free(error);
    }
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_USER
    );
    g_object_unref(provider);
}*/
void Navigateur::chargerStyles() {
    GtkCssProvider *provider = gtk_css_provider_new();
    GError *error = NULL;

    std::string cheminCSS = GestionnaireFichiers::cheminStylesCSS();
    gtk_css_provider_load_from_path(provider, cheminCSS.c_str(), &error);

    if (error) {
        g_warning("Erreur de chargement du CSS : %s", error->message);
        g_error_free(error);
    }

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

void Navigateur::onAjouterFavori(GtkButton *, Navigateur *n) {
    std::string url = n->moteurRendu->obtenirURLActuelle();
    if (!url.empty()) {
        n->ajouterFavori("Favori", url, "");
        n->rafraichirBarreFavoris();
    }
}

void Navigateur::onBarreURLActivate(GtkEntry *entry, Navigateur *n) {
    n->chargerURL(gtk_entry_get_text(entry));
}

void Navigateur::onCliqueFavori(GtkButton *, Navigateur *n, const std::string& url) {
    n->chargerURL(url);
}

void Navigateur::fermerApplication() {
    sauvegarderConfiguration();
    if (fenetre) gtk_widget_destroy(fenetre);
    gtk_main_quit();
}


void Navigateur::onNaviguerRetourWrapper(GtkButton *button, gpointer user_data) {
    auto *navigateur = static_cast<Navigateur*>(user_data);
    if (navigateur) {
        navigateur->onNaviguerRetour(button, navigateur);
    }
}
