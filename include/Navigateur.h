#ifndef NAVIGATEUR_H
#define NAVIGATEUR_H

#include <memory>
#include <gtk/gtk.h>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "MoteurRendu.h"
#include "GestionnaireHTTP.h"
#include "GestionnaireMemoire.h"
#include "MoteurScript.h"
#include "GestionnaireFavoris.h"
#include <set>

class Navigateur {
public:
    Navigateur();
    ~Navigateur();

    // Méthode principales
    void lancer();
    void chargerURL(const std::string& url);
    nlohmann::json& getFavoris();
    
    // Gestion des favoris via GestionnaireFavoris
    void afficherGestionnaireFavoris();
    void rafraichirBarreFavoris();

    void ajouterFavori(const std::string& nom, const std::string& url, const std::string& tag);

    // Interface utilisateur
    void fermerApplication();
    void sauvegarderConfiguration();
    void ajouterNouvelOnglet(const std::string &url = "");
    void changerOngletActif(GtkWidget* ongletWidget);
    void afficherMessage(const std::string& message);
    void afficherParametres();
    void configurerRaccourcisClavier();


    void onCliqueFavori(GtkButton*, Navigateur*, const std::string&);
    void executerScriptDansOngletActif(const std::string& script);

    std::string getURLActuelle() const;

    // Méthodes internes liés à GTK
    static void onNaviguerRetourWrapper(GtkButton *button, gpointer user_data);
    static void onNaviguerSuivantWrapper(GtkButton *button, gpointer user_data);
    static void onRafraichirPageWrapper(GtkButton *button, gpointer user_data);
    static void onBarreURLActivate(GtkEntry *entry, Navigateur *navigateur);
    static void onCliqueFavoriWrapper(GtkButton *button, gpointer user_data);

    // Autres méthode publiques non indiquer par toi ChatGPT..
    //void supprimerOnglet(GtkWidget *ongletWidget);
    static void onNaviguerRetour(GtkButton *button, Navigateur *navigateur);
    static void onNaviguerSuivant(GtkButton *button, Navigateur *navigateur);
    static void onRafraichirPage(GtkButton *button, Navigateur *navigateur);
    static void onAllerAccueil(GtkButton *button, Navigateur *navigateur);
    static void onAjouterFavori(GtkButton*, Navigateur*);
    static void onBoutonFavorisClicked(GtkButton* button, gpointer user_data);


private:
    // Méthodes internes
    void construireInterface();
    void chargerConfiguration();
    // void sauvegarderConfiguration();
    void initialiserBarreNavigation();
    void creerMenuContextuel(GtkWidget* bouton);
    void initialiserBarreFavoris();
    void initialiserBarreOnglets();
    void mettreEnSurbrillance(GtkWidget* ongletWidget);
    void chargerStyles();
    void ajouterBouton(GtkWidget* conteneur, const std::string& iconName, GCallback callback, gpointer data);

    // Gestion des onglets
    void supprimerOnglet(GtkWidget* ongletWidget);

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
    GtkWidget *barreFavoris = nullptr;

    std::vector<std::pair<std::string, GtkWidget*>> onglets;
    std::vector<std::string> historique;

    std::string homepage;
    nlohmann::json favoris; // Favoris stockés sous forme de JSON
};

#endif
