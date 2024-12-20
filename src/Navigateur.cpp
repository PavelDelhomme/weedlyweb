#include "Navigateur.h"
#include "MoteurRendu.h"
#include "GestionnaireHTTP.h"
#include "GestionnaireMemoire.h"
#include <iostream>
#include <fstream>
#include <string>
#include <nlohmann/json.hpp>

Navigateur::Navigateur() {
    moteurRendu = new MoteurRendu();
    gestionnaireMemoire = new GestionnaireMemoire();
    gestionnaireHTTP = new GestionnaireHTTP();
    gestionnaireMemoire->initialiserSurveillance();
    chargerConfiguration();
    construireInterface();
}

void Navigateur::construireInterface() {
    // Création de la fenêtre principale
    fenetre = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(fenetre), "WeedlyWeb");
    gtk_window_set_default_size(GTK_WINDOW(fenetre), 1024, 768);
    g_signal_connect(fenetre, "destroy", G_CALLBACK(+[](GtkWidget *widget, gpointer data) {
        Navigateur *navigateur = static_cast<Navigateur*>(data);
        navigateur->fermerApplication();
    }), this);

    // Barre d'URL
    barreURL = gtk_entry_new();
    g_signal_connect(barreURL, "activate", G_CALLBACK(on_barre_url_active), this);

    // Configuration de WebKitSettings
    vueWeb = webkit_web_view_new();
    WebKitSettings *settings = webkit_web_view_get_settings(WEBKIT_WEB_VIEW(vueWeb));
    g_object_set(G_OBJECT(settings), 
        "enable-javascript", FALSE, 
        "media-playback-requires-user-gesture", TRUE,
        "enable-accelerated-2d-canvas", TRUE,
        NULL);

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
    g_signal_connect(boutonAccueil, "clicked", G_CALLBACK(on_button_accueil_clicked), this);
    g_signal_connect(boutonMenuOptions, "clicked", G_CALLBACK(on_bouton_parametres_clicked), this);

    GtkWidget *barreNavigation = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(barreNavigation), boutonRetour, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(barreNavigation), boutonSuivant, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(barreNavigation), boutonAccueil, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(barreNavigation), boutonRecharger, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(barreNavigation), barreURL, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(barreNavigation), boutonEtoileFavori, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(barreNavigation), boutonMenuOptions, FALSE, FALSE, 0);

    // Barre de favoris
    barreFavoris = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

    // Conteneur principal
    GtkWidget *conteneurPrincipal = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(conteneurPrincipal), barreNavigation, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(conteneurPrincipal), barreFavoris, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(conteneurPrincipal), vueWeb, TRUE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(fenetre), conteneurPrincipal);
}

void Navigateur::lancer() {
    gtk_widget_show_all(fenetre);
    moteurRendu->afficherPage();
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


void Navigateur::on_favoris_loaded(GObject *source_object, GAsyncResult *res, gpointer user_data) {
    GError *error = NULL;
    gsize length;
    char *contents = NULL;

    if (g_file_load_contents_finish(G_FILE(source_object), res, &contents, &length, NULL, &error)) {
        // Traiter le contenu de 'contents'
        std::cout << "📁 Favoris chargés avec succès" << std::endl;
    } else {
        std::cerr << "❌ Erreur de lecture du fichier des favoris : " << error->message << std::endl;
        g_error_free(error);
    }

    g_free(contents);
}

void Navigateur::ajouterFavori(const std::string& nom, const std::string& url, const std::string& tag) {
    try {
        // Lecture du fichier favoris.json
        nlohmann::json favoris;
        GFile *file = g_file_new_for_path("favoris.json");
        g_file_load_contents_async(file, NULL, on_favoris_loaded, this);

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

void Navigateur::on_button_accueil_clicked(GtkButton *button, Navigateur *navigateur) {
    navigateur->chargerURL(navigateur->homepage.c_str());
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
    if (load_event != WEBKIT_LOAD_FINISHED) return; // 🟢 Ignore tous les autres événements
    const gchar *url = webkit_web_view_get_uri(WEBKIT_WEB_VIEW(web_view));
    navigateur->mettreAJourURLBarre(url);
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

void Navigateur::fermerApplication() {
    gestionnaireMemoire->optimiserMemoire();
    if (vueWeb) g_object_unref(vueWeb);
    if (fenetre) gtk_widget_destroy(fenetre);
    delete moteurRendu;
    delete gestionnaireMemoire;
    delete gestionnaireHTTP;
    gtk_main_quit();
}
