#ifndef MOTEURRENDU_H
#define MOTEURRENDU_H

#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#include <string>

class MoteurRendu {
public:
    MoteurRendu();
    ~MoteurRendu();

    void initialiserRendu(GtkWidget *conteneurPrincipal);
    void afficherPage(const std::string& url);
    void rafraichirPage();
    void naviguerRetour();
    void naviguerSuivant();
    GtkWidget* creerBouton(const std::string& label, GCallback callback, gpointer data);
    GtkWidget* creerChampTexte(GCallback callback, gpointer data);

    std::string obtenirURLActuelle() const;
    
private:
    WebKitWebView *vueWeb;
};

#endif
