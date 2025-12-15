#ifndef NAVIGATEUR_H
#define NAVIGATEUR_H

#include <memory>
#include <string>
#include <vector>
#include <set>
#include <nlohmann/json.hpp>
#include <gtk/gtk.h>
#include "MoteurRendu.h"
#include "GestionnaireHTTP.h"
#include "GestionnaireMemoire.h"
#include "MoteurScript.h"
#include "GestionnaireFavoris.h"
#include "GestionnaireOnglets.h"
#include "utils/CommandPalette.h"
#include "utils/RequestInterceptor.h"
#include "database/Database.h"

class Navigateur {
public:
    Navigateur();
    ~Navigateur();

    // Méthodes principales
    void lancer();
    void sauvegarderConfiguration();
    void chargerConfiguration();

    // Gestion de l'interface
    void construireInterface();
    void chargerStyles();

    // Gestion des onglets
    GestionnaireOnglets* getGestionnaireOnglets() {
       return gestionnaireOnglets.get();
    }
    void changerGroupeOnglets(const std::string& nomGroupe);
    void ajouterNouvelOnglet(const std::string &url = "");
    void supprimerOnglet(GtkWidget* ongletWidget);
    void changerOngletActif(GtkWidget* ongletWidget);

    // Gestion des favoris
    void initialiserBarreFavoris();
    void rafraichirBarreFavoris();
    void afficherGestionnaireFavoris();
    void afficherMenuFavoris();
    void afficherMenuFavorisRestants();
    void ajouterFavori(const std::string& nom, const std::string& url, const std::string& tag);
    void supprimerFavori(GtkWidget* widget);
    void afficherPaletteCommandes();

    // Méthodes utilitaires
    void chargerURL(const std::string& url);
    std::string getURLActuelle() const;
    std::string getTitreActuel() const;
    
    // Méthodes internes liés à GTK
    static void onNaviguerRetourWrapper(GtkButton *button, gpointer user_data);
    static void onNaviguerSuivantWrapper(GtkButton *button, gpointer user_data);
    static void onRafraichirPageWrapper(GtkButton *button, gpointer user_data);
    static void onBarreURLActivate(GtkEntry *entry, gpointer user_data);
    void onNaviguerRetour(GtkButton *button, Navigateur* navigateur);
    void onNaviguerSuivant(GtkButton *button, Navigateur* navigateur);
    void onRafraichirPage(GtkButton * button, Navigateur* navigateur);
    void onAllerAccueil(GtkButton * button, Navigateur* navigateur);

    // Getters/Setters
    std::shared_ptr<nlohmann::json> getFavoris();
    std::string getHomepage() const { return homepage; }
    
    GtkWidget* getEntryNomFavori() { return entryNomFavori; }
    GtkWidget* getEntryURLFavori() { return entryURLFavori; }
    GtkWidget* getPopoverFavoris() { return popoverFavoris; }

    // Interface utilisateur
    void fermerApplication();


private:
    // Membres d'interface utilisateur
    GtkWidget *fenetre;
    GtkWidget *conteneurPrincipal;
    GtkWidget *barreNavigation;
    GtkWidget *barreFavoris;
    GtkWidget *barreOnglets;
    GtkWidget *barreURL;
    GtkWidget *boutonEtoile;
    GtkWidget *entryNomFavori;
    GtkWidget *entryURLFavori;
    GtkWidget *popoverFavoris;

    // Composants internes
    std::unique_ptr<MoteurRendu> moteurRendu;
    std::unique_ptr<GestionnaireHTTP> gestionnaireHTTP;
    std::unique_ptr<GestionnaireMemoire> gestionnaireMemoire;
    std::unique_ptr<MoteurScript> moteurScript;
    std::unique_ptr<GestionnaireFavoris> gestionnaireFavoris;
    std::unique_ptr<GestionnaireOnglets> gestionnaireOnglets;
    std::unique_ptr<CommandPalette> commandPalette;
    std::unique_ptr<RequestInterceptor> requestInterceptor;
    std::unique_ptr<Database> database;

    // Données
    std::vector<std::string> historique;
    std::vector<std::pair<std::string, GtkWidget*>> onglets;
    std::shared_ptr<nlohmann::json> favoris;
    std::string homepage;

    // Méthodes internes
    void initialiserBarreNavigation();
    void initialiserBarreOnglets();
    void initialiserPopoverFavoris();
    void mettreEnSurbrillance(GtkWidget* ongletWidget);
    void ajouterBouton(GtkWidget* conteneur, const std::string& iconName, GCallback callback, gpointer data);
    void mettreAJourBoutonEtoile();
    void executerScriptDansOngletActif(const std::string& script);
    void afficherMessage(const std::string& message);
    void afficherParametres();
    void creerMenuContextuel(GtkWidget* bouton);
    static void onBoutonFavorisClicked(GtkButton* button, gpointer user_data);

    // Gestionnaire de mémoire et signaux
    void configurerRaccourcisClavier(); 
};

#endif
