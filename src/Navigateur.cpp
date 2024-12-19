#include "Navigateur.h"
#include <iostream>
#include <fstream>
#include <string>
#include <nlohmann/json.hpp>

Navigateur::Navigateur() {
    chargerConfiguration();
    construireInterface();
}

void Navigateur::construireInterface() {
    // Création de la fenêtre principale
    fenetre = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(fenetre), "WeedlyWeb");
    gtk_window_set_default_size(GTK_WINDOW(fenetre), 1024, 768);
    g_signal_connect(fenetre, "destroy", G_CALLBACK(gtk_main_quit), nullptr);

    // Barre d'URL
    barreURL = gtk_entry_new();
    g_signal_connect(barreURL, "activate", G_CALLBACK(on_barre_url_active), this);


    // Boutons de navigation
    boutonEtoileFavori = gtk_button_new_with_label("⭐");
    boutonMenuOptions = gtk_button_new_with_label("☰");
    boutonAccueil = gtk_button_new_with_label("🏠");
    boutonRecharger = gtk_button_new_with_label("🔄");

    GtkWidget *boutonRetour = gtk_button_new_with_label("◀️");
    GtkWidget *boutonSuivant = gtk_button_new_with_label("▶️");

    g_signal_connect(boutonRetour, "clicked", G_CALLBACK(on_bouton_retour_clicked), this);
    g_signal_connect(boutonSuivant, "clicked", G_CALLBACK(on_bouton_suivant_clicked), this);
    g_signal_connect(boutonRecharger, "clicked", G_CALLBACK(on_bouton_recharger_clicked), this);
    g_signal_connect(boutonAccueil, "clicked", G_CALLBACK(on_bouton_accueil_clicked), this);
    g_signal_connect(boutonMenuOptions, "clicked", G_CALLBACK(on_bouton_parametres_clicked), this);

    GtkWidget *barreNavigation = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(barreNavigation), boutonRetour, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(barreNavigation), boutonSuivant, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(barreNavigation), boutonAccueil, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(barreNavigation), boutonRecharger, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(barreNavigation), barreURL, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(barreNavigation), boutonEtoileFavori, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(barreNavigation), boutonMenuOptions, FALSE, FALSE, 0);

    // Vue Web
    vueWeb = webkit_web_view_new();
    g_signal_connect(vueWeb, "load-changed", G_CALLBACK(on_load_changed), this);
 
    // Conteneur principal
    GtkWidget *conteneurPrincipal = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(conteneurPrincipal), barreNavigation, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(conteneurPrincipal), vueWeb, TRUE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(fenetre), conteneurPrincipal);
}

void Navigateur::lancer() {
    gtk_widget_show_all(fenetre);
    chargerURL(homepage.c_str());
}

void Navigateur::chargerURL(const char* url) {
    webkit_web_view_load_uri(WEBKIT_WEB_VIEW(vueWeb), url);
    historique.push_back(url);
    gtk_entry_set_text(GTK_ENTRY(barreURL), url);
}


void Navigateur::on_etoile_favori_clicked(GtkButton *button, Navigateur *navigateur) {
    const gchar *url = webkit_web_view_get_uri(WEBKIT_WEB_VIEW(navigateur->vueWeb));
    navigateur->ajouterFavori("Favori", url, "Aucun");
}



void Navigateur::ajouterFavori(const std::string& nom, const std::string& url, const std::string& tag) {
    try {
        // Lecture du fichier favoris.json
        nlohmann::json favoris;
        std::ifstream fichierLecture("favoris.json");

        // Si le fichier n'existe pas, on le crée avec un tableau vide
        if (!fichierLecture) {
            std::ofstream fichierParDefaut("favoris.json");
            fichierParDefaut << "[]" << std::endl;
            fichierParDefaut.close();
        } else {
            fichierLecture >> favoris;
            fichierLecture.close();
        }

        // Vérifie si le favori existe déjà (par URL) pour éviter les doublons
        bool existeDeja = false;
        for (const auto& favori : favoris) {
            if (favori["url"] == url) {
                existeDeja = true;
                std::cerr << "⚠️ Le favori existe déjà : " << url << std::endl;
                break;
            }
        }

        // Si le favori n'existe pas, on l'ajoute
        if (!existeDeja) {
            favoris.push_back({{"name", nom}, {"url", url}, {"tag", tag}});

            // Écriture des favoris mis à jour dans le fichier
            std::ofstream fichierEcriture("favoris.json");
            fichierEcriture << favoris.dump(4); // 4 = indentation
            fichierEcriture.close();

            std::cout << "✅ Favori ajouté avec succès : " << nom << " (" << url << ")" << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "❌ Erreur d'ajout au fichier des favoris : " << e.what() << std::endl;
    }
}


void Navigateur::afficherHistorique() {
    std::cout << "Historique de navigation :" << std::endl;
    for (const auto& url : historique) {
        std::cout << url << std::endl;
    }
}


void Navigateur::ouvrirParametres() {
    GtkWidget *dialogue = gtk_dialog_new_with_buttons("Paramètres", GTK_WINDOW(fenetre), GTK_DIALOG_MODAL, "_Fermer", GTK_RESPONSE_CLOSE, NULL);
    GtkWidget *conteneurDialogue = gtk_dialog_get_content_area(GTK_DIALOG(dialogue));
    GtkWidget *texte = gtk_label_new("Page de paramètres");
    gtk_box_pack_start(GTK_BOX(conteneurDialogue), texte, TRUE, TRUE, 0);
    gtk_widget_show_all(dialogue);
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

void Navigateur::on_bouton_parametres_clicked(GtkButton *button, Navigateur *navigateur) {
    navigateur->ouvrirParametres();
}

void Navigateur::on_load_changed(WebKitWebView *web_view, WebKitLoadEvent load_event, Navigateur *navigateur) {
    if (load_event == WEBKIT_LOAD_FINISHED) {
        const gchar *url = webkit_web_view_get_uri(WEBKIT_WEB_VIEW(web_view));
        navigateur->mettreAJourURLBarre(url);
    }
}

void Navigateur::on_load_failed(WebKitWebView *web_view, WebKitLoadEvent load_event, const gchar *failing_uri, GError *error, Navigateur *navigateur) {
    std::cerr << "Erreur de chargement de la page : " << failing_uri << std::endl;
}


void Navigateur::mettreAJourURLBarre(const gchar* url) {
    gtk_entry_set_text(GTK_ENTRY(barreURL), url);
}

void Navigateur::chargerConfiguration() {
    try {
        std::ifstream fichier("config.json");
        nlohmann::json config;
        if (!fichier) {
            std::ofstream fichierParDefaut("config.json");
            config["homepage"] = "https://www.duckduckgo.com";
            fichierParDefaut << config.dump(4);
            fichierParDefaut.close();
        } else {
            fichier >> config;
        }
        homepage = config.value("homepage", "https://www.duckduckgo.com");
    } catch (const std::exception& e) {
        std::cerr << "Erreur de lecture de la configuration : " << e.what() << std::endl;
        homepage = "https://www.duckduckgo.com";
    }
}
