#ifndef NAVIGATEUR_H
#define NAVIGATEUR_H

#include <webkit2/webkit2.h>
#include <gtk/gtk.h>

class Navigateur {
public:
    Navigateur();
    void lancer();
    void chargerURL(const char* url);
    void afficherMessage(const char* message);

private:
    GtkWidget *fenetre;
    GtkWidget *vueWeb;
    GtkWidget *barreURL;
    GtkWidget *buttonsNavigation;

    // Fonctions de navigation
    static void on_barre_url_active(GtkEntry *entry, Navigateur *navigateur);
    static void on_bouton_retour_clicked(GtkButton *button, Navigateur *navigateur);
    static void on_bouton_suivant_clicked(GtkButton *button, Navigateur *navigateur);
    static void on_bouton_recharger_clicked(GtkButton *button, Navigateur *navigateur);
    static void on_load_failed(WebKitWebView *web_view, WebKitLoadEvent load_event, const gchar *failing_uri, GError *error, Navigateur *navigateur);
};

#endif
