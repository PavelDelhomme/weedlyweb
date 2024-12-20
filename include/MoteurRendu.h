#ifndef MOTEURRENDU_H
#define MOTEURRENDU_H

#include <webkit2/webkit2.h>
#include <gtk/gtk.h>
#include <string>

class MoteurRendu {
public:
    MoteurRendu();
    ~MoteurRendu();

    void initialiserRendu(GtkWidget *conteneur);
    void afficherPage(const std::string& url);
    void afficherHTML(const std::string& contenuHTML);
    void rafraichirPage();
    void arreterChargement();
    void allerEnArriere();
    void allerEnAvant();
    std::string obtenirURLActuelle() const;
    void definirParametresParDefaut();
    void activerJavaScript(bool activer);
    void activerImages(bool activer);
    void activerCache(bool activer);
    
private:
    WebKitWebView *vueWeb;
};

#endif
