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

class Navigateur {
public:
    Navigateur();
    ~Navigateur();

    void lancer();
    void chargerURL(const std::string& url);
    void ajouterFavori(const std::string& nom, const std::string& url, const std::string& tag);
    void fermerApplication();
    void ajouterNouvelOnglet(const std::string &url);
    void afficherMessage(const std::string& message);
    void afficherParametres();

    //void chargerConfiguration();
    //void sauvegarderConfiguration();
    //void afficherHistorique();
    //void afficherFavoris();
    
private:
    // Méthodes internes
    void construireInterface();
    void chargerConfiguration();
    void sauvegarderConfiguration();
    void chargerFavoris();
    void sauvegarderFavoris();
    void initialiserBarreNavigation();
    void initialiserBarreOnglets();

    // Membres
    std::unique_ptr<MoteurRendu> moteurRendu;
    std::unique_ptr<GestionnaireHTTP> gestionnaireHTTP;
    std::unique_ptr<GestionnaireMemoire> gestionnaireMemoire;

    GtkWidget *fenetre = nullptr;
    GtkWidget *conteneurPrincipal = nullptr;
    GtkWidget *barreURL = nullptr;
    GtkWidget *barreNavigation = nullptr;
    GtkWidget *barreOnglets = nullptr;

    std::vector<std::pair<std::string, GtkWidget*>> onglets;
    std::vector<std::string> historique;

    //GtkWidget *boutonAccueil = nullptr;

    std::string homepage;
    nlohmann::json favoris; // Au lieu de std::vector<nlohmann::json>
};

#endif
