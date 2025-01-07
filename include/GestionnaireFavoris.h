#ifndef GESTIONNAIREFAVORIS_H
#define GESTIONNAIREFAVORIS_H

#include <gtk/gtk.h>
#include <nlohmann/json.hpp>
#include <string>
#include <functional>

class GestionnaireFavoris {
public:
    GestionnaireFavoris(nlohmann::json& favoris, std::function<void()> callbackRafraichir);
    ~GestionnaireFavoris();

    void afficherFenetre();
    void rafraichirInterface();
    void ajouterFavori(const std::string& nom, const std::string& url, const std::string& tag);
    void ajouterDossier(const std::string& nom);
    void supprimerFavori(const std::string& nomFavori);
    void modifierFavori(const std::string& nomFavori, const std::string& nouvelURL);
    void ajouterFavoriDansDossier(const std::string& dossier, const std::string& nom, const std::string& url);
    void creerMenuContextuelFavori(GtkWidget* bouton, const std::string& nomFavori);

private:
    GtkWidget *fenetre = nullptr;
    GtkWidget *listeFavoris = nullptr;
    GtkWidget *formulaireModification = nullptr;
    nlohmann::json& favoris;
    std::function<void()> callbackRafraichir;

    void creerInterface();
    void sauvegarderModifications();
    void afficherListeFavoris();
    static void on_bouton_fermer_clicked(GtkWidget*, gpointer user_data);
};

#endif
