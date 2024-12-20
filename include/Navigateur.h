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
    void afficherMessage(const std::string& message);
    void chargerConfiguration();
    void sauvegarderConfiguration();
    void ajouterFavori(const std::string& nom, const std::string& url, const std::string& tag);
    void afficherHistorique();
    void afficherFavoris();
    void fermerApplication();

private:
    std::unique_ptr<MoteurRendu> moteurRendu;
    std::unique_ptr<GestionnaireHTTP> gestionnaireHTTP;
    std::unique_ptr<GestionnaireMemoire> gestionnaireMemoire;

    GtkWidget *fenetre = nullptr;
    GtkWidget *barreURL = nullptr;
    GtkWidget *boutonAccueil = nullptr;

    std::string homepage;
    nlohmann::json favoris; // Au lieu de std::vector<nlohmann::json>
    std::vector<std::string> historique;

    void construireInterface();
    void chargerFavoris();
    void sauvegarderFavoris();
};

#endif
