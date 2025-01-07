#include "GestionnaireFavoris.h"
#include "GestionnaireFichiers.h"
#include <iostream>
#include <algorithm>
#include <gtk/gtk.h>

GestionnaireFavoris::GestionnaireFavoris(nlohmann::json& favoris, std::function<void()> callbackRafraichir)
    : favoris(favoris), callbackRafraichir(callbackRafraichir) {
    creerInterface();
}

GestionnaireFavoris::~GestionnaireFavoris() {
    if (fenetre) gtk_widget_destroy(fenetre);
}

static void on_bouton_fermer_clicked(GtkWidget*, gpointer user_data) {
    auto* gestionnaire = static_cast<GestionnaireFavoris*>(user_data);
    if (gestionnaire && gestionnaire->fenetre) {
        gtk_widget_destroy(gestionnaire->fenetre);
    }
}

void GestionnaireFavoris::creerInterface() {
    fenetre = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(fenetre), "Gestionnaire de Favoris");
    gtk_window_set_default_size(GTK_WINDOW(fenetre), 600, 400);

    GtkWidget *conteneurPrincipal = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(fenetre), conteneurPrincipal);

    // Liste des favoris
    listeFavoris = gtk_tree_view_new();
    gtk_box_pack_start(GTK_BOX(conteneurPrincipal), listeFavoris, TRUE, TRUE, 0);

    // Bouton de fermeture
    GtkWidget *boutonFermer = gtk_button_new_with_label("Fermer");
    g_signal_connect(boutonFermer, "clicked", G_CALLBACK(on_bouton_fermer_clicked), this);
    gtk_box_pack_start(GTK_BOX(conteneurPrincipal), boutonFermer, FALSE, FALSE, 0);

    afficherListeFavoris();
    gtk_widget_show_all(fenetre);
}

void GestionnaireFavoris::afficherListeFavoris() {
    GtkListStore *store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);

    // Parcourir et afficher les favoris
    for (const auto& favori : favoris) {
        if (favori.contains("name") && favori.contains("url")) {
            GtkTreeIter iter;
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter,
                               0, favori["name"].get<std::string>().c_str(),
                               1, favori["url"].get<std::string>().c_str(),
                               -1);
        }
    }

    // Configuration des colonnes
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *colNom = gtk_tree_view_column_new_with_attributes("Nom", renderer, "text", 0, NULL);
    GtkTreeViewColumn *colURL = gtk_tree_view_column_new_with_attributes("URL", renderer, "text", 1, NULL);

    gtk_tree_view_append_column(GTK_TREE_VIEW(listeFavoris), colNom);
    gtk_tree_view_append_column(GTK_TREE_VIEW(listeFavoris), colURL);
    gtk_tree_view_set_model(GTK_TREE_VIEW(listeFavoris), GTK_TREE_MODEL(store));
    g_object_unref(store); // Libérer correctement la mémoire
}

void GestionnaireFavoris::ajouterFavori(const std::string& nom, const std::string& url, const std::string& tag) {
    favoris.push_back({{"name", nom}, {"url", url}, {"tag", tag}});
    callbackRafraichir();
    rafraichirInterface();
}

void GestionnaireFavoris::ajouterDossier(const std::string& nom) {
    favoris.push_back({{"name", nom}, {"type", "folder"}, {"children", nlohmann::json::array()}});
    callbackRafraichir();
    rafraichirInterface();
}

void GestionnaireFavoris::supprimerFavori(const std::string& nomFavori) {
    auto it = std::remove_if(favoris.begin(), favoris.end(), [&](const nlohmann::json& favori) {
        return favori["name"] == nomFavori;
    });

    if (it != favoris.end()) {
        favoris.erase(it, favoris.end());
        callbackRafraichir();
        rafraichirInterface();
    } else {
        std::cerr << "Favori introuvable : " << nomFavori << std::endl;
    }
}

void GestionnaireFavoris::modifierFavori(const std::string& nomFavori, const std::string& nouvelURL) {
    for (auto& favori : favoris) {
        if (favori["name"] == nomFavori) {
            favori["url"] = nouvelURL;
            callbackRafraichir();
            rafraichirInterface();
            return;
        }
    }
    std::cerr << "Favori non trouvé : " << nomFavori << std::endl;
}

void GestionnaireFavoris::creerMenuContextuelFavori(GtkWidget* bouton, const std::string& nomFavori) {
    GtkWidget *menu = gtk_menu_new();

    // Modifier le favori
    GtkWidget *modifierItem = gtk_menu_item_new_with_label("Modifier");
    g_signal_connect(modifierItem, "activate", G_CALLBACK(+[](GtkWidget*, gpointer data) {
        auto *gf = static_cast<GestionnaireFavoris*>(data);
        std::string nomFavoriCapture = "nomFavori";
        gf->modifierFavori(nomFavoriCapture, "https://nouvelle-url.com");
    }), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), modifierItem);

    // Supprimer le favori
    GtkWidget *supprimerItem = gtk_menu_item_new_with_label("Supprimer");
    g_signal_connect(supprimerItem, "activate", G_CALLBACK(+[](GtkWidget*, gpointer user_data) {
        auto* pair = static_cast<std::pair<GestionnaireFavoris*, std::string>*>(user_data);
        pair->first->supprimerFavori(pair->second);
    }), new std::pair<GestionnaireFavoris*, std::string>(this, nomFavori));

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), supprimerItem);

    gtk_widget_show_all(menu);
    gtk_menu_popup_at_widget(GTK_MENU(menu), bouton, GDK_GRAVITY_SOUTH_WEST, GDK_GRAVITY_NORTH_WEST, nullptr);
}

void GestionnaireFavoris::sauvegarderModifications() {
    GestionnaireFichiers::ecrireJSON(GestionnaireFichiers::cheminFavorisJSON(), favoris);
}

void GestionnaireFavoris::rafraichirInterface() {
    gtk_widget_destroy(listeFavoris); 
    listeFavoris = gtk_tree_view_new();  // Réinitialisation complète
    afficherListeFavoris();
}

void GestionnaireFavoris::afficherFenetre() {
    gtk_widget_show_all(fenetre);
}
