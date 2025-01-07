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
#include <set>

class Navigateur {
public:
    Navigateur();
    ~Navigateur();

    void lancer();
    void chargerURL(const std::string& url);
    void ajouterFavori(const std::string& nom, const std::string& url, const std::string& tag);
    void fermerApplication();
    void ajouterNouvelOnglet(const std::string &url = "");
    void changerOngletActif(GtkWidget* ongletWidget);
    static void onCliqueFavoriWrapper(GtkButton* button, gpointer user_data);
    static void onNaviguerSuivantWrapper(GtkButton *button, gpointer user_data);
    static void onRafraichirPageWrapper(GtkButton *button, gpointer user_data);
    void supprimerOnglet(GtkWidget *ongletWidget);
    void afficherFavoris(const nlohmann::json& favoris);
    void afficherMessage(const std::string& message);
    void afficherParametres();
    void configurerRaccourcisClavier();

    void creerDossierFavoris(const std::string& nom);
    void deplacerFavori(const std::string& nomFavori, const std::string& dossierDestination);
    void rafraichirBarreFavoris(); // Met à jour l'interface après un déplacement ou une création
    // Nouvelle méthode pour naviguer dans un dossier de favoris
    void naviguerDansDossier(const std::string& dossier);
    void creerMenuContextuelFavoris(GtkWidget* bouton, const nlohmann::json& favori);
    void ajouterFavoriDansDossier(const std::string& dossier, const std::string& nom, const std::string& url, const std::string& tag);


private:
    // Méthodes internes
    void construireInterface();
    void afficherGestionnaireFavoris();
    void ajouterDossierFavoris(const std::string&);
    void modifierFavori(const nlohmann::json&);
    void supprimerFavori(const nlohmann::json&);
    void chargerConfiguration();
    void sauvegarderConfiguration();
    void chargerFavoris();
    void sauvegarderFavoris();
    void initialiserBarreNavigation();
    void initialiserBarreOnglets();
    void initialiserBarreFavoris();
    void mettreEnSurbrillance(GtkWidget* ongletWidget);
    void chargerStyles();
    void ajouterBouton(GtkWidget* conteneur, const std::string& iconName, GCallback callback, gpointer data);
    void executerScriptDansOngletActif(const std::string& script);
    static void onCliqueFavori(GtkButton *button, Navigateur *navigateur, const std::string& url);
    static void onNaviguerRetour(GtkButton *button, Navigateur *navigateur);
    static void onNaviguerSuivant(GtkButton *button, Navigateur *navigateur);
    static void onRafraichirPage(GtkButton *button, Navigateur *navigateur);
    static void onAllerAccueil(GtkButton *button, Navigateur *navigateur);
    static void onAjouterFavori(GtkButton *button, Navigateur *navigateur);
    static void onBarreURLActivate(GtkEntry *entry, Navigateur *navigateur);
    static void onNaviguerRetourWrapper(GtkButton *button, gpointer user_data);

    // Membres
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
