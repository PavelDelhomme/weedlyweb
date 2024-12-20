#include "MoteurRendu.h"
#include <iostream>

MoteurRendu::MoteurRendu() {
    vueWeb = WEBKIT_WEB_VIEW(webkit_web_view_new());
}

MoteurRendu::~MoteurRendu() {
    if (vueWeb) {
        g_clear_object(&vueWeb);
    }
}


void MoteurRendu::initialiserRendu(GtkWidget *fenetre, GtkWidget *barreURL) {
    GtkWidget *conteneur = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(conteneur), GTK_WIDGET(vueWeb), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(conteneur), barreURL, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(fenetre), conteneur);
}

void MoteurRendu::afficherPage(const std::string& url) {
    webkit_web_view_load_uri(vueWeb, url.c_str());
}

std::string MoteurRendu::obtenirURLActuelle() const {
    const gchar *url = webkit_web_view_get_uri(vueWeb);
    return url ? std::string(url) : "";
}

GtkWidget* MoteurRendu::creerBouton(const std::string& label, GCallback callback, gpointer data) {
    GtkWidget *bouton = gtk_button_new_with_label(label.c_str());
    g_signal_connect(bouton, "clicked", callback, data);
    return bouton;
}

GtkWidget* MoteurRendu::creerChampTexte(GCallback callback, gpointer data) {
    GtkWidget *champ = gtk_entry_new();
    g_signal_connect(champ, "activate", callback, data);
    return champ;
}

