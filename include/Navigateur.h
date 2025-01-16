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
#include "GestionnaireFavoris.h"
#include "GestionnaireOnglets.h"

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

    // Méthodes utilitaires
    void chargerURL(const std::string& url);
    std::string getURLActuelle() const;
    std::string getTitreActuel() const;

    
    // Getters/Setters
    std::shared_ptr<nlohmann::json> getFavoris();
    std::string getHomepage() const { return homepage; }
    GtkWidget* getEntryNomFavori() const { return entryNomFavori; }


private:
    // Membres d'interface utilisateur
    GtkWidget *fenetre;
    GtkWidget *conteneurPrincipal;
    GtkWidget *barreNavigation;
    GtkWidget *barreFavoris;
    GtkWidget *barreOnglets;
    GtkWidget *entryNomFavori;
    GtkWidget *entryURLFavori;
    GtkWidget *popoverFavoris;

    // Composants internes
    std::unique_ptr<MoteurRendu> moteurRendu;
    std::unique_ptr<GestionnaireHTTP> gestionnaireHTTP;
    std::unique_ptr<GestionnaireFavoris> gestionnaireFavoris;
    std::unique_ptr<GestionnaireOnglets> gestionnaireOnglets;

    // Données
    std::vector<std::pair<std::string, GtkWidget*>> onglets;
    std::shared_ptr<nlohmann::json> favoris;
    std::string homepage;

    // Méthodes internes
    void initialiserBarreNavigation();
    void initialiserPopoverFavoris();
    void mettreEnSurbrillance(GtkWidget* ongletWidget);

    // Gestionnaire de mémoire et signaux
    void configurerRaccourcisClavier(); 
};

#endif
