#ifndef GESTIONNAIREFAVORIS_H
#define GESTIONNAIREFAVORIS_H

#include <gtk/gtk.h>
#include <nlohmann/json.hpp>
#include <string>

class GestionnaireFavoris {
public:
    GestionnaireFavoris(nlohmann::json& favoris, std::function<void()> callbackRafraichir);
    ~GestionnaireFavoris();

    void afficherFenetre();
    void rafraichirInterface();

    void creerMenuContextuelFavori(GtkWidget* bouton, const std::string& nomFavori);
    void modifierFavori(const std::string& nomFavori, const std::string& nouvelURL);
    void ajouterFavoriDansDossier(const std::string& dossier, const std::string& nom, const std::string& url);

private:
    GtkWidget *fenetre = nullptr;
    GtkWidget *listeFavoris = nullptr;
    GtkWidget *formulaireModification = nullptr;
    nlohmann::json& favoris; // Référence pour sauvegarder les modifications directement
    std::function<void()> callbackRafraichir; // Callback pour rafraîchir la barre de favoris dans le navigateur

    void creerInterface();
    void afficherListeFavoris();
    void afficherDetailsFavori(const nlohmann::json& favori);
    void sauvegarderModifications();
    void supprimerFavori(const std::string& nomFavori);
    void ajouterFavori(const std::string& nom, const std::string& url, const std::string& tag);
    void ajouterDossier(const std::string& nom);
};

#endif
