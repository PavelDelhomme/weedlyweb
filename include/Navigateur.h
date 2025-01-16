#ifndef NAVIGATEUR_H
#define NAVIGATEUR_H

#include <memory>
#include <gtk/gtk.h>
#include <glib-object.h>
#include <string>
#include <set>
#include <vector>
#include <nlohmann/json.hpp>
#include "MoteurRendu.h"
#include "GestionnaireHTTP.h"
#include "GestionnaireMemoire.h"
#include "MoteurScript.h"
#include "GestionnaireFavoris.h"
#include "GestionnaireOnglets.h"

class Navigateur {
public:
    Navigateur();
    ~Navigateur();

    // Gestion de l'interface
    void lancer();
    void construireInterface();
    void sauvegarderConfiguration();
    void chargerConfiguration();

    // Gestion des onglets
    void ajouterNouvelOnglet(const std::string &url = "");
    void changerOngletActif(GtkWidget* ongletWidget);
    void supprimerOnglet(GtkWidget* ongletWidget);


    // Gestion des favoris via GestionnaireFavoris
    void initialiserBarreFavoris();
    void rafraichirBarreFavoris();
    void afficherMenuFavoris();
    void afficherMenuFavorisRestants();
    void afficherGestionnaireFavoris();
    void ajouterFavori(const std::string& nom, const std::string& url, const std::string& tag);
    void supprimerFavori(GtkWidget* widget);

    // Méthodes utilitaires
    void chargerURL(const std::string& url);
    std::string getURLActuelle() const;
    std::string getTitreActuel() const;

    // Getter/Setter
    GtkWidget* getEntryNomFavori() { return entryNomFavori; }
    GtkWidget* getEntryURLFavori() { return entryURLFavori; }
    GtkWidget* getPopoverFavoris() { return popoverFavoris; }

    nlohmann::json& getFavoris();

    std::string getHomepage() const { return homepage; }



    // Gestion des onglets
    //GestionnaireOnglets* getGestionnaireOnglets() {
    //    return gestionnaireOnglets.get();
    //}
    //void changerGroupeOnglets(const std::string& nomGroupe);


    // Interface utilisateur
    //void fermerApplication();
    // void afficherMessage(const std::string& message);
    // void afficherParametres();
    // void configurerRaccourcisClavier();
    // void creerMenuContextuel(GtkWidget* bouton);

    // void onCliqueFavori(GtkButton*, Navigateur*, const std::string&);
    // void executerScriptDansOngletActif(const std::string& script);

    // void mettreAJourBoutonEtoile();

    // static void on_supprimer_favori(GtkWidget*, gpointer user_data);



    // // Méthodes internes liés à GTK
    // static void onNaviguerRetourWrapper(GtkButton *button, gpointer user_data);
    // static void onNaviguerSuivantWrapper(GtkButton *button, gpointer user_data);
    // static void onRafraichirPageWrapper(GtkButton *button, gpointer user_data);
    // static void onBarreURLActivate(GtkEntry *entry, gpointer user_data);

    // static void onCliqueFavoriWrapper(GtkButton *button, gpointer user_data);

    // static void onNaviguerRetour(GtkButton *button, Navigateur *navigateur);
    // static void onNaviguerSuivant(GtkButton *button, Navigateur *navigateur);
    // static void onRafraichirPage(GtkButton *button, Navigateur *navigateur);
    // static void onAllerAccueil(GtkButton *button, Navigateur *navigateur);
    // static void onAjouterFavori(GtkButton*, Navigateur*);
    // static void onBoutonFavorisClicked(GtkButton* button, gpointer user_data);

    // static void on_ouvrir_nouvel_onglet_safe(GtkWidget*, gpointer user_data);
    // static void on_modifier_favori(GtkWidget*, gpointer user_data);

private:
    // Membres d'interface
    GtkWidget *fenetre;
    GtkWidget *conteneurPrincipal;
    GtkWidget *barreNavigation;
    GtkWidget *barreFavoris;
    GtkWidget *barreOnglets;
    GtkWidget *entryNomFavori;
    GtkWidget *entryURLFavori;
    GtkWidget *popoverFavoris;

    std::unique_ptr<MoteurRendu> moteurRendu;
    std::unique_ptr<GestionnaireHTTP> gestionnaireHTTP;
    std::unique_ptr<GestionnaireFavoris> gestionnaireFavoris;
    std::unique_ptr<GestionnaireOnglets> gestionnaireOnglets;

    std::vector<std::pair<std::string, GtkWidget*>> onglets;
    std::shared_ptr<nlohmann::json> favoris;
    std::string homepage;

    // Méthode utilitaires internes
    void initialiserBarreNavigation();
    void initialiserPopoverFavoris();
    void creerMenuContextuel(GtkWidget* bouton);
    void chargerStyles();

    // void initialiserBarreNavigation();
    // void initialiserBarreOnglets();
    // void mettreEnSurbrillance(GtkWidget* ongletWidget);
    // void chargerStyles();
    // void ajouterBouton(GtkWidget* conteneur, const std::string& iconName, GCallback callback, gpointer data);

    // void initialiserPopoverFavoris();

    // // Gestion des onglets
    // std::unique_ptr<GestionnaireOnglets> gestionnaireOnglets;

    // // Gestion des favoris via GestionnaireFavoris
    // std::unique_ptr<GestionnaireFavoris> gestionnaireFavoris;

    // // Membres privés
    // std::unique_ptr<MoteurRendu> moteurRendu;
    // std::unique_ptr<GestionnaireHTTP> gestionnaireHTTP;
    // std::unique_ptr<GestionnaireMemoire> gestionnaireMemoire;
    // std::unique_ptr<MoteurScript> moteurScript;

    // GtkWidget *fenetre = nullptr;
    // GtkWidget *conteneurPrincipal = nullptr;
    // GtkWidget *barreURL = nullptr;
    // GtkWidget *barreNavigation = nullptr;
    // GtkWidget *barreOnglets = nullptr;
    // GtkWidget* boutonEtoile = nullptr;


    // // Favoris
    // GtkWidget *barreFavoris = nullptr;
    // GtkWidget* popoverFavoris;
    // GtkWidget* entryNomFavori;
    // GtkWidget* entryURLFavori;

    // std::vector<std::pair<std::string, GtkWidget*>> onglets;
    // std::vector<std::string> historique;

    // std::string homepage;
    // // nlohmann::json favoris; // Favoris stockés sous forme de JSON
    // std::shared_ptr<nlohmann::json> favoris = std::make_shared<nlohmann::json>();
};

#endif
