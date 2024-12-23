#include "Navigateur.h"
#include <iostream>
#include <fstream>

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
    initialiserBarreNavigation();
    moteurRendu->initialiserRendu(conteneurPrincipal);

    g_signal_connect(fenetre, "destroy", G_CALLBACK(+[](GtkWidget *, gpointer data) {
        static_cast<Navigateur *>(data)->fermerApplication();
    }), this);

    gtk_widget_show_all(fenetre);
}


void Navigateur::initialiserBarreNavigation() {
    barreNavigation = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

    auto ajouterBouton = [this](const std::string &label, GCallback callback) {
        GtkWidget *bouton = moteurRendu->creerBouton(label, callback, this);
        gtk_box_pack_start(GTK_BOX(barreNavigation), bouton, FALSE, FALSE, 0);
    };

    ajouterBouton("Retour", G_CALLBACK(+[](GtkButton *, Navigateur *n) { n->moteurRendu->naviguerRetour(); }));
    ajouterBouton("Suivant", G_CALLBACK(+[](GtkButton *, Navigateur *n) { n->moteurRendu->naviguerSuivant(); }));
    ajouterBouton("Recharger", G_CALLBACK(+[](GtkButton *, Navigateur *n) { n->chargerURL(n->moteurRendu->obtenirURLActuelle()); }));
    ajouterBouton("Accueil", G_CALLBACK(+[](GtkButton *, Navigateur *n) { n->chargerURL(n->homepage); }));

    barreURL = moteurRendu->creerChampTexte(G_CALLBACK(+[](GtkEntry *entry, Navigateur *n) {
        n->chargerURL(gtk_entry_get_text(entry));
    }), this);

    gtk_box_pack_start(GTK_BOX(barreNavigation), barreURL, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(conteneurPrincipal), barreNavigation, FALSE, FALSE, 0);
}


void Navigateur::initialiserBarreOnglets() {
    barreOnglets = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    ajouterNouvelOnglet(homepage);
    gtk_box_pack_start(GTK_BOX(conteneurPrincipal), barreOnglets, FALSE, FALSE, 0);
}

void Navigateur::ajouterNouvelOnglet(const std::string &url) {
    GtkWidget *boutonOnglet = moteurRendu->creerBouton("Nouvel onglet", G_CALLBACK(+[](GtkButton *, Navigateur *n) {
        n->chargerURL(n->homepage);
    }), this);

    onglets.push_back({url, boutonOnglet});
    gtk_box_pack_start(GTK_BOX(barreOnglets), boutonOnglet, FALSE, FALSE, 0);
    gtk_widget_show_all(barreOnglets);
}


void Navigateur::chargerURL(const std::string& url) {
    moteurRendu->afficherPage(url);
    historique.push_back(url);
}

void Navigateur::afficherMessage(const std::string& message) {
    std::cout << "Message : " << message << std::endl;
}

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
}


void Navigateur::sauvegarderConfiguration() {
    nlohmann::json config;
    config["homepage"] = homepage;
    std::ofstream fichier("config.json");
    fichier << config.dump(4);
}

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
}

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


void Navigateur::ajouterFavori(const std::string& nom, const std::string& url, const std::string& tag) {
    favoris.push_back({{"name", nom}, {"url", url}, {"tag", tag}});
    sauvegarderFavoris();
}



void Navigateur::afficherParametres() {
    // Logique pour afficher une page de configuration
    moteurRendu->afficherPage("file://parametres.html");
}


void Navigateur::fermerApplication() {
    sauvegarderConfiguration();
    gtk_main_quit();
}