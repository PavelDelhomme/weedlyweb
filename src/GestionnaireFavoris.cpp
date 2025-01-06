#include "GestionnaireFavoris.h"
#include <iostream>

GestionnaireFavoris::GestionnaireFavoris(nlohmann::json& favoris, std::function<void()> callbackRafraichir)
    : favoris(favoris), callbackRafraichir(callbackRafraichir) {
    creerInterface();
}

GestionnaireFavoris::~GestionnaireFavoris() {
    if (fenetre) gtk_widget_destroy(fenetre);
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

    // Formulaire de modification
    formulaireModification = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(conteneurPrincipal), formulaireModification, FALSE, FALSE, 0);

    GtkWidget *boutonSauvegarder = gtk_button_new_with_label("Sauvegarder");
    g_signal_connect(boutonSauvegarder, "clicked", G_CALLBACK(+[](GtkWidget*, GestionnaireFavoris* gf) {
        gf->sauvegarderModifications();
    }), this);
    gtk_box_pack_start(GTK_BOX(conteneurPrincipal), boutonSauvegarder, FALSE, FALSE, 0);

    afficherListeFavoris();
    gtk_widget_show_all(fenetre);
}

void GestionnaireFavoris::creerMenuContextuelFavori(GtkWidget* bouton, const std::string& nomFavori) {
    GtkWidget *menu = gtk_menu_new();

    GtkWidget *modifierItem = gtk_menu_item_new_with_label("Modifier");
    g_signal_connect(modifierItem, "activate", G_CALLBACK(+[](GtkWidget*, GestionnaireFavoris* gf, const std::string nom) {
        gf->supprimerFavori(nom); 
    }), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), modifierItem);

    GtkWidget *supprimerItem = gtk_menu_item_new_with_label("Supprimer");
    g_signal_connect(supprimerItem, "activate", G_CALLBACK(+[](GtkWidget*, GestionnaireFavoris* gf, const std::string nom) {
        gf->supprimerFavori(nom);
    }), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), supprimerItem);

    gtk_widget_show_all(menu);
    gtk_menu_popup_at_widget(GTK_MENU(menu), bouton, GDK_GRAVITY_SOUTH_WEST, GDK_GRAVITY_NORTH_WEST, nullptr);
}

void GestionnaireFavoris::modifierFavori(const std::string& nomFavori, const std::string& nouvelURL) {
    for (auto& favori : favoris) {
        if (favori["name"] == nomFavori) {
            favori["url"] = nouvelURL;
            sauvegarderModifications();
            break;
        }
    }
    callbackRafraichir();
}
void GestionnaireFavoris::ajouterFavoriDansDossier(const std::string& dossier, const std::string& nom, const std::string& url) {
    for (auto& favori : favoris) {
        if (favori["name"] == dossier && favori["type"] == "folder") {
            favori["children"].push_back({{"name", nom}, {"url", url}});
            callbackRafraichir();
            return;
        }
    }
    std::cerr << "Dossier introuvable : " << dossier << std::endl;
}


void GestionnaireFavoris::afficherListeFavoris() {
    GtkListStore *store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);

    // Parcourir les favoris et ajouter les données au modèle
    for (const auto& favori : favoris) {
        GtkTreeIter iter;
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                           0, favori["name"].get<std::string>().c_str(),
                           1, favori["url"].get<std::string>().c_str(),
                           -1);
    }

    // Associer le modèle au TreeView
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *colNom = gtk_tree_view_column_new_with_attributes("Nom", renderer, "text", 0, NULL);
    GtkTreeViewColumn *colURL = gtk_tree_view_column_new_with_attributes("URL", renderer, "text", 1, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(listeFavoris), colNom);
    gtk_tree_view_append_column(GTK_TREE_VIEW(listeFavoris), colURL);

    gtk_tree_view_set_model(GTK_TREE_VIEW(listeFavoris), GTK_TREE_MODEL(store));
    g_object_unref(store); // Libérer le modèle
}


void GestionnaireFavoris::sauvegarderModifications() {
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(listeFavoris));
    GtkTreeModel *model;
    GtkTreeIter iter;

    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        gchar *nom;
        gchar *url;
        gtk_tree_model_get(model, &iter, 0, &nom, 1, &url, -1);

        for (auto& favori : favoris) {
            if (favori["name"] == nom) {
                favori["url"] = url;
                callbackRafraichir(); // Appelle la méthode pour sauvegarder et rafraîchir
                break;
            }
        }

        g_free(nom);
        g_free(url);
    }
}



void GestionnaireFavoris::supprimerFavori(const std::string& nomFavori) {
    favoris.erase(std::remove_if(favoris.begin(), favoris.end(), [&](const nlohmann::json& favori) {
        return favori["name"] == nomFavori;
    }), favoris.end());

    callbackRafraichir(); // Utilise le callback pour sauvegarder et rafraîchir
    rafraichirInterface();
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

void GestionnaireFavoris::afficherFenetre() {
    gtk_widget_show_all(fenetre);
}

void GestionnaireFavoris::rafraichirInterface() {
    gtk_widget_destroy(listeFavoris);
    afficherListeFavoris();
}
