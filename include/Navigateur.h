#ifndef NAVIGATEUR_H
#define NAVIGATEUR_H

#include <webkit2/webkit2.h>
#include <gtk/gtk.h>
#include <string>
#include <vector>

class Navigateur {
public:
    Navigateur();
    void lancer();
    void chargerURL(const char* url);
    void afficherMessage(const char* message);
    void chargerConfiguration();
    void ajouterFavori(const std::string& nom, const std::string& url, const std::string& tag);
    void afficherHistorique();
    void afficherFavoris();
    void ouvrirParametres();
    void mettreAJourURLBarre(const gchar* url);
    void chargerPageAccueil();
    void fermerApplication();

private:
    MoteurRendu *moteurRendu;
    GtkWidget *fenetre;
    GtkWidget *vueWeb;
    GtkWidget *barreURL;
    GtkWidget *boutonsNavigation;
    GtkWidget *barreFavoris;
    GtkWidget *historiqueWidget;
    GtkWidget *boutonEtoileFavori;
    GtkWidget *boutonMenuOptions;
    GtkWidget *boutonAccueil;
    GtkWidget *boutonRecharger;

    std::string homepage;
    std::vector<std::string> historique;

    // Fonctions de navigation
    static void on_barre_url_active(GtkEntry *entry, Navigateur *navigateur);
    static void on_bouton_retour_clicked(GtkButton *button, Navigateur *navigateur);
    static void on_bouton_suivant_clicked(GtkButton *button, Navigateur *navigateur);
    static void on_bouton_recharger_clicked(GtkButton *button, Navigateur *navigateur);
    static void on_button_accueil_clicked(GtkButton *button, Navigateur *navigateur);
    static void on_load_changed(WebKitWebView *web_view, WebKitLoadEvent load_event, Navigateur *navigateur);
    static void on_etoile_favori_clicked(GtkButton *button, Navigateur *navigateur);
    static void on_bouton_parametres_clicked(GtkButton *button, Navigateur *navigateur);
    static void on_load_failed(WebKitWebView *web_view, WebKitLoadEvent load_event, const gchar *failing_uri, GError *error, Navigateur *navigateur); // ✅ Ajouté cette déclaration manquante

    void construireInterface();
};

#endif
