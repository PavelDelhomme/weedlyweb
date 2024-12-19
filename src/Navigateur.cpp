#include "Navigateur.h"
#include <iostream>

Navigateur::Navigateur() {
    // Création de la fenêtre principale
    fenetre = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(fenetre), "WeedlyWeb");
    gtk_window_set_default_size(GTK_WINDOW(fenetre), 1024, 768);

    // Liaison de la fermeture de la fenêtre au signal "destroy"
    g_signal_connect(fenetre, "destroy", G_CALLBACK(gtk_main_quit), nullptr);

    // Barre d'URL
    barreURL = gtk_entry_new();
    g_signal_connect(barreURL, "activate", G_CALLBACK(on_barre_url_active), this);

    // Boutons de navigation
    GtkWidget *boutonRetour = gtk_button_new_with_label("◀️");
    g_signal_connect(boutonRetour, "clicked", G_CALLBACK(on_bouton_retour_clicked), this);

    GtkWidget *boutonSuivant = gtk_button_new_with_label("▶️");
    g_signal_connect(boutonSuivant, "clicked", G_CALLBACK(on_bouton_suivant_clicked), this);

    GtkWidget *boutonRecharger = gtk_button_new_with_label("🔄");
    g_signal_connect(boutonRecharger, "clicked", G_CALLBACK(on_bouton_recharger_clicked), this);

    GtkWidget *barreNavigation = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(barreNavigation), boutonRetour, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(barreNavigation), boutonSuivant, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(barreNavigation), boutonRecharger, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(barreNavigation), barreURL, TRUE, TRUE, 0);

    // Vue Web
    vueWeb = webkit_web_view_new();
    g_signal_connect(vueWeb, "load-failed", G_CALLBACK(on_load_failed), this);

    GtkWidget *conteneurPrincipal = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(conteneurPrincipal), barreNavigation, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(conteneurPrincipal), vueWeb, TRUE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(fenetre), conteneurPrincipal);
}

void Navigateur::lancer() {
    gtk_widget_show_all(fenetre);
    chargerURL("https://www.duckduckgo.com");
}

void Navigateur::chargerURL(const char* url) {
    webkit_web_view_load_uri(WEBKIT_WEB_VIEW(vueWeb), url);
}

void Navigateur::afficherMessage(const char* message) {
    std::cout << "Message : " << message << std::endl;
}

void Navigateur::on_barre_url_active(GtkEntry *entry, Navigateur *navigateur) {
    const char *url = gtk_entry_get_text(entry);
    navigateur->chargerURL(url);
}

void Navigateur::on_bouton_retour_clicked(GtkButton *button, Navigateur *navigateur) {
    webkit_web_view_go_back(WEBKIT_WEB_VIEW(navigateur->vueWeb));
}

void Navigateur::on_bouton_suivant_clicked(GtkButton *button, Navigateur *navigateur) {
    webkit_web_view_go_forward(WEBKIT_WEB_VIEW(navigateur->vueWeb));
}

void Navigateur::on_bouton_recharger_clicked(GtkButton *button, Navigateur *navigateur) {
    webkit_web_view_reload(WEBKIT_WEB_VIEW(navigateur->vueWeb));
}

void Navigateur::on_load_failed(WebKitWebView *web_view, WebKitLoadEvent load_event, const gchar *failing_uri, GError *error, Navigateur *navigateur) {
    std::cerr << "Erreur de chargement de la page : " << failing_uri << std::endl;
}
