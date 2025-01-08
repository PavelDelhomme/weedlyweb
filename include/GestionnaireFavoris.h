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
    void creerInterface();
    void afficherListeFavoris();
    void ajouterFavori(const std::string& nom, const std::string& url, const std::string& tag);
    void supprimerFavori(const std::string& nomFavori);
    void modifierFavori(const std::string& nomFavori, const std::string& nouvelURL);
    void creerMenuContextuelFavori(GtkWidget* bouton, const std::string& nomFavori);
    void sauvegarderModifications();
    void rafraichirInterface();
    void afficherFenetre();
    void fermerFenetre();
    void ajouterDossier(const std::string& nom);
    // void ajouterFavoriDansDossier(const std::string& dossier, const std::string& nom, const std::string& url);
    GtkWidget* getListeFavoris();  // Déclaration d'un accesseur
    
private:
    nlohmann::json& favoris;
    std::function<void()> callbackRafraichir;
    GtkWidget* fenetre;
    GtkWidget* listeFavoris;
    // GtkWidget *fenetre = nullptr;
    // GtkWidget *listeFavoris = nullptr;
    // GtkWidget *formulaireModification = nullptr;
    // nlohmann::json& favoris;
    // std::function<void()> callbackRafraichir;
    // static void on_bouton_fermer_clicked(GtkWidget*, gpointer user_data);
};

#endif
