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
    
    // Gestion des favoris
    /*void ajouterFavori(const std::string& nom, const std::string& url, const std::string& tag);
    void creerDossierFavoris(const std::string& nom);
    void ajouterFavoriDansDossier(const std::string& dossier, const std::string& nom, const std::string& url, const std::string& tag);
    void deplacerFavori(const std::string& nomFavori, const std::string& dossierDestination);
    void supprimerFavori(const nlohmann::json& favori);
    void afficherFavoris(const nlohmann::json& favoris);
    void naviguerDansDossier(const std::string& dossier);*/
    void rafraichirBarreFavoris();

    // Interface utilisateur
    void fermerApplication();
    void ajouterNouvelOnglet(const std::string &url = "");
    void changerOngletActif(GtkWidget* ongletWidget);
    void afficherMessage(const std::string& message);
    void afficherParametres();
    void configurerRaccourcisClavier();

    // Méthodes internes liés à GTK
    static void onNaviguerRetourWrapper(GtkButton *button, gpointer user_data);
    static void onNaviguerSuivantWrapper(GtkButton *button, gpointer user_data);
    static void onRafraichirPageWrapper(GtkButton *button, gpointer user_data);
    static void onBarreURLActivate(GtkEntry *entry, Navigateur *navigateur);
    static void onCliqueFavoriWrapper(GtkButton *button, gpointer user_data);

    // Autres méthode publiques non indiquer par toi ChatGPT..
    void supprimerOnglet(GtkWidget *ongletWidget);
    static void onNaviguerRetour(GtkButton *button, Navigateur *navigateur);
    static void onNaviguerSuivant(GtkButton *button, Navigateur *navigateur);
    static void onRafraichirPage(GtkButton *button, Navigateur *navigateur);
    static void onAllerAccueil(GtkButton *button, Navigateur *navigateur);
    void onAjouterFavori(GtkButton*, Navigateur*);
    void onCliqueFavori(GtkButton*, Navigateur*, const std::string&);

private:
    // Méthodes internes
    void construireInterface();
    void chargerConfiguration();
    void sauvegarderConfiguration();
    void initialiserBarreNavigation();
    void initialiserBarreFavoris();
    void initialiserBarreOnglets();
    void mettreEnSurbrillance(GtkWidget* ongletWidget);
    void chargerStyles();
    void ajouterBouton(GtkWidget* conteneur, const std::string& iconName, GCallback callback, gpointer data);

    void creerMenuContextuelFavoris(GtkWidget* bouton, const nlohmann::json& favori);
    void executerScriptDansOngletActif(const std::string& script);

    // Gestion des favoris
    void chargerFavoris();
    void sauvegarderFavoris();
    void modifierFavori(const nlohmann::json& favori);

    // Gestion des onglets
    void supprimerOnglet(GtkWidget* ongletWidget);

    // Autres méthode privée non indiquer par toi ChatGPT...
    void ajouterDossierFavoris(const std::string& nom);
    void supprimerFavori(const nlohmann::json& favori);
    void afficherGestionnaireFavoris();


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
