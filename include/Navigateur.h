#ifndef NAVIGATEUR_H
#define NAVIGATEUR_H

#include <memory>
#include <gtk/gtk.h>
#include <glib-object.h>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "MoteurRendu.h"
#include "GestionnaireHTTP.h"
#include "GestionnaireMemoire.h"
#include "MoteurScript.h"
#include "GestionnaireFavoris.h"
#include "GestionnaireOnglets.h"
#include <set>

class Navigateur {
public:
    Navigateur();
    ~Navigateur();

    // Méthode principales
    void lancer();
    void chargerURL(const std::string& url);
    nlohmann::json& getFavoris();

    std::string getHomepage() const { return homepage; }
    GtkWidget* getEntryURLFavori() const { return entryURLFavori; }
    GtkWidget* getPopoverFavoris() const { return popoverFavoris; }
    GtkWidget* getEntryNomFavori() const { return entryNomFavori; }


    // Gestion des onglets
    GestionnaireOnglets* getGestionnaireOnglets() {
        return gestionnaireOnglets.get();
    }
    void changerGroupeOnglets(const std::string& nomGroupe);
    void ajouterNouvelOnglet(const std::string &url = "");
    void changerOngletActif(GtkWidget* ongletWidget);


    // Gestion des favoris via GestionnaireFavoris
    void afficherGestionnaireFavoris();
    void rafraichirBarreFavoris();

    void ajouterFavori(const std::string& nom, const std::string& url, const std::string& tag);

    // Interface utilisateur
    void fermerApplication();
    void sauvegarderConfiguration();
    void afficherMessage(const std::string& message);
    void afficherParametres();
    void configurerRaccourcisClavier();
    void creerMenuContextuel(GtkWidget* bouton);

    void afficherMenuFavoris();
    void supprimerFavori(GtkWidget* widget);
    void onCliqueFavori(GtkButton*, Navigateur*, const std::string&);
    void executerScriptDansOngletActif(const std::string& script);

    void mettreAJourBoutonEtoile();

    static void on_supprimer_favori(GtkWidget*, gpointer user_data);


    std::string getURLActuelle() const;

    // Méthodes internes liés à GTK
    static void onNaviguerRetourWrapper(GtkButton *button, gpointer user_data);
    static void onNaviguerSuivantWrapper(GtkButton *button, gpointer user_data);
    static void onRafraichirPageWrapper(GtkButton *button, gpointer user_data);
    static void onBarreURLActivate(GtkEntry *entry, gpointer user_data);

    static void onCliqueFavoriWrapper(GtkButton *button, gpointer user_data);

    static void onNaviguerRetour(GtkButton *button, Navigateur *navigateur);
    static void onNaviguerSuivant(GtkButton *button, Navigateur *navigateur);
    static void onRafraichirPage(GtkButton *button, Navigateur *navigateur);
    static void onAllerAccueil(GtkButton *button, Navigateur *navigateur);
    static void onAjouterFavori(GtkButton*, Navigateur*);
    static void onBoutonFavorisClicked(GtkButton* button, gpointer user_data);

    static void on_ouvrir_nouvel_onglet_safe(GtkWidget*, gpointer user_data);
    static void on_modifier_favori(GtkWidget*, gpointer user_data);

private:
    // Méthodes internes
    void construireInterface();
    void chargerConfiguration();
    // void sauvegarderConfiguration();
    void initialiserBarreNavigation();
    void initialiserBarreFavoris();
    void initialiserBarreOnglets();
    void mettreEnSurbrillance(GtkWidget* ongletWidget);
    void chargerStyles();
    void ajouterBouton(GtkWidget* conteneur, const std::string& iconName, GCallback callback, gpointer data);

    void initialiserPopoverFavoris();

    // Gestion des onglets
    void supprimerOnglet(GtkWidget* ongletWidget);
    std::unique_ptr<GestionnaireOnglets> gestionnaireOnglets;

    // Gestion des favoris via GestionnaireFavoris
    std::unique_ptr<GestionnaireFavoris> gestionnaireFavoris;

    // Membres privés
    std::unique_ptr<MoteurRendu> moteurRendu;
    std::unique_ptr<GestionnaireHTTP> gestionnaireHTTP;
    std::unique_ptr<GestionnaireMemoire> gestionnaireMemoire;
    std::unique_ptr<MoteurScript> moteurScript;

    GtkWidget *fenetre = nullptr;
    GtkWidget *conteneurPrincipal = nullptr;
    GtkWidget *barreURL = nullptr;
    GtkWidget *barreNavigation = nullptr;
    GtkWidget *barreOnglets = nullptr;
    GtkWidget* boutonEtoile = nullptr;


    // Favoris
    GtkWidget *barreFavoris = nullptr;
    GtkWidget* popoverFavoris;
    GtkWidget* entryNomFavori;
    GtkWidget* entryURLFavori;

    std::vector<std::pair<std::string, GtkWidget*>> onglets;
    std::vector<std::string> historique;

    std::string homepage;
    // nlohmann::json favoris; // Favoris stockés sous forme de JSON
    std::shared_ptr<nlohmann::json> favoris = std::make_shared<nlohmann::json>();
};

#endif
