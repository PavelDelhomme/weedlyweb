#include "managers/FavoritesManager.h"
#include "utils/Utils.h"
#include "managers/FileManager.h"
#include <iostream>
#include <algorithm>
#include <gtk/gtk.h>

FavoritesManager::FavoritesManager(std::shared_ptr<nlohmann::json> favorites, std::function<void()> callbackRafraichir)
    : favorites(std::move(favorites)), callbackRafraichir(callbackRafraichir) {
    createInterface();
}

FavoritesManager::~FavoritesManager() {
    if (window && GTK_IS_WIDGET(window)) {
        gtk_widget_destroy(window);
        window = nullptr;
    }
}

void FavoritesManager::closeWindow() {
    if (window && GTK_IS_WIDGET(window)) {
        gtk_widget_destroy(window);
        window = nullptr;
    }
}

static void on_bouton_fermer_clicked(GtkWidget*, gpointer user_data) {
    auto* gestionnaire = static_cast<FavoritesManager*>(user_data);
    if (gestionnaire) {
        gestionnaire->closeWindow();
    }
}

static void on_supprimer_favori(GtkWidget*, gpointer user_data) {
    auto* gestionnaire = static_cast<FavoritesManager*>(user_data);
    
    // Utilisation de l'accesseur public
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gestionnaire->getListeFavoris()));
    GtkTreeModel *model;
    GtkTreeIter iter;

    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        gchar *favoriteName;
        gtk_tree_model_get(model, &iter, 0, &favoriteName, -1); // Colonne 0 = Nom du favori
        gestionnaire->removeFavorite(favoriteName);
        g_free(favoriteName);
    } else {
        std::cerr << "Aucun favori sélectionné pour suppression." << std::endl;
    }
}


void FavoritesManager::createInterface() {
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Gestionnaire de Favoris");
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);

    GtkWidget *mainContainer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(window), mainContainer);

    // Liste des favorites
    favoritesList = gtk_tree_view_new();
    gtk_box_pack_start(GTK_BOX(mainContainer), favoritesList, TRUE, TRUE, 0);

    // Bouton de fermeture
    GtkWidget *boutonFermer = gtk_button_new_with_label("Fermer");
    g_signal_connect(boutonFermer, "clicked", G_CALLBACK(on_bouton_fermer_clicked), this);
    gtk_box_pack_start(GTK_BOX(mainContainer), boutonFermer, FALSE, FALSE, 0);
    
    // Bouton d'ajout
    GtkWidget *boutonAjouterFavori = gtk_button_new_with_label("Ajouter Favori");
    g_signal_connect(boutonAjouterFavori, "clicked", G_CALLBACK(+[](GtkWidget*, gpointer user_data) {
        auto* gestionnaire = static_cast<FavoritesManager*>(user_data);
        if (gestionnaire) {
            gestionnaire->addFavorite("Nouveau Favori", "https::example.com", "Exemple");
        }
    }), this);
    gtk_box_pack_start(GTK_BOX(mainContainer), boutonAjouterFavori, FALSE, FALSE, 0);

    // Bouton supprimer (Correction : déplacement ici dans le constructeur)
    GtkWidget *supprimerItem = gtk_button_new_with_label("Supprimer");
    g_signal_connect(supprimerItem, "clicked", G_CALLBACK(on_supprimer_favori), this);
    gtk_box_pack_start(GTK_BOX(mainContainer), supprimerItem, FALSE, FALSE, 0);

    displayFavoritesList();
    // Ne pas afficher la fenêtre automatiquement - elle sera affichée via showWindow()
}

void FavoritesManager::displayFavoritesList() {
    GtkListStore *store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);

    // Parcourir et afficher les favorites
    for (const auto& favori : *favorites) {
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

    gtk_tree_view_append_column(GTK_TREE_VIEW(favoritesList), colNom);
    gtk_tree_view_append_column(GTK_TREE_VIEW(favoritesList), colURL);
    gtk_tree_view_set_model(GTK_TREE_VIEW(favoritesList), GTK_TREE_MODEL(store));
    g_object_unref(store); // Libérer correctement la mémoire
}

void FavoritesManager::addFolder(const std::string& nom) {
    (*favorites).push_back({{"name", nom}, {"type", "folder"}, {"children", nlohmann::json::array()}});
    if (favorites->empty()) {

    }
    callbackRafraichir();
    refreshInterface();
}

void FavoritesManager::addFavorite(const std::string& nom, const std::string& url, const std::string& tag) {
    // Vérifie si un favori avec le même nom ou la même URL existe déjà
    auto doublon = std::find_if(favorites->begin(), favorites->end(), [&nom, &url](const nlohmann::json& favori) {
        return (favori.contains("name") && favori["name"] == nom) || 
               (favori.contains("url") && favori["url"] == url);
    });

    if (doublon != favorites->end()) {
        std::cerr << "Le favori existe déjà : " << nom << " (" << url << ")" << std::endl;
        return;
    }

    // Ajout du favori
    favorites->push_back({{"name", nom}, {"url", url}, {"tag", tag}});

    // Sauvegarde et mise à jour de l'interface
    saveModifications();
    callbackRafraichir();
    refreshInterface();
}

void FavoritesManager::removeFavorite(const std::string& favoriteName) {
    auto it = std::remove_if(favorites->begin(), favorites->end(), [&](const nlohmann::json& favori) {
        return favori.contains("name") && favori["name"] == favoriteName;
    });
    
    if (it != favorites->end()) {
        favorites->erase(it, favorites->end());
        saveModifications();
        afficherMessageConsole("Favori supprimé avec succès : " + favoriteName);
        callbackRafraichir();  // Rafraîchir l'interface pour refléter les changements
        refreshInterface();
    } else {
        afficherMessageConsole("Erreur : Impossible de trouver le favori : " + favoriteName);
    }
}



bool FavoritesManager::removeFavoriteFromList(const std::string& favoriteName) {
    auto it = std::remove_if(favorites->begin(), favorites->end(), [&favoriteName](const nlohmann::json& favori) {
        return favori.contains("name") && favori["name"] == favoriteName;
    });

    if (it != favorites->end()) {
        favorites->erase(it, favorites->end());
        return true;
    }
    return false;
}

void FavoritesManager::modifyFavorite(const std::string& favoriteName, const std::string& newUrl) {
    auto it = std::find_if(favorites->begin(), favorites->end(), [&favoriteName](const nlohmann::json& favori) {
        return favori.contains("name") && favori["name"] == favoriteName;
    });

    if (it != favorites->end()) {
        (*it)["url"] = newUrl;
        saveModifications();
        afficherMessageConsole("Favori modifié : " + favoriteName + " -> " + newUrl);
        callbackRafraichir();  // Rafraîchir pour afficher les modifications
        refreshInterface();
    } else {
        afficherMessageConsole("Erreur : Favori non trouvé pour modification : " + favoriteName);
    }
}

void FavoritesManager::createFavoriteContextMenu(GtkWidget* button, const std::string& favoriteName) {
    GtkWidget* menu = gtk_menu_new();

    // Ouvrir dans un nouvel onglet
    GtkWidget* ouvrirNouvelOngletItem = gtk_menu_item_new_with_label("Ouvrir dans un nouvel onglet");
    g_signal_connect(ouvrirNouvelOngletItem, "activate", G_CALLBACK(+[](GtkWidget*, gpointer user_data) {
        auto* gestionnaire = static_cast<FavoritesManager*>(user_data);
        gestionnaire->callbackRafraichir();
    }), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), ouvrirNouvelOngletItem);

    // Modifier le favori
    GtkWidget* modifierItem = gtk_menu_item_new_with_label("Modifier");
    g_signal_connect(modifierItem, "activate", G_CALLBACK(+[](GtkWidget*, gpointer user_data) {
        auto* gestionnaire = static_cast<FavoritesManager*>(user_data);
        gestionnaire->modifyFavorite(static_cast<const char*>(g_object_get_data(G_OBJECT(user_data), "favoriteName")), "https://example.com");
    }), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), modifierItem);

    // Supprimer le favori
    GtkWidget* supprimerItem = gtk_menu_item_new_with_label("Supprimer");
    g_object_set_data_full(G_OBJECT(supprimerItem), "favoriteName", g_strdup(favoriteName.c_str()), g_free);
    g_signal_connect(supprimerItem, "activate", G_CALLBACK(+[](GtkWidget* widget, gpointer user_data) {
        auto* gestionnaire = static_cast<FavoritesManager*>(user_data);
        const char* favoriteName = static_cast<const char*>(g_object_get_data(G_OBJECT(widget), "favoriteName"));
        gestionnaire->removeFavorite(favoriteName);
    }), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), supprimerItem);

    gtk_widget_show_all(menu);
    gtk_menu_popup_at_widget(GTK_MENU(menu), button, GDK_GRAVITY_SOUTH_WEST, GDK_GRAVITY_NORTH_WEST, nullptr);
}


void FavoritesManager::saveModifications() {
    FileManager::writeJSON(FileManager::favoritesJSONPath(), *favorites);
}

void FavoritesManager::refreshInterface() {
    if (favoritesList && GTK_IS_WIDGET(favoritesList)) {
        // Vérifier que le widget n'est pas déjà en cours de destruction
        if (!gtk_widget_in_destruction(favoritesList)) {
            // Retirer du container avant de détruire
            GtkWidget* parent = gtk_widget_get_parent(favoritesList);
            if (parent && GTK_IS_CONTAINER(parent)) {
                gtk_container_remove(GTK_CONTAINER(parent), favoritesList);
            }
            gtk_widget_destroy(favoritesList);
        }
        favoritesList = nullptr;
    }
    favoritesList = gtk_tree_view_new();  // Réinitialisation complète de la liste
    displayFavoritesList();  // Recharge les favorites actualisés
    if (window && GTK_IS_WIDGET(window)) {
        gtk_widget_show_all(window);  // Affiche tous les widgets mis à jour
    }
}

GtkWidget* FavoritesManager::getListeFavoris() {
    return favoritesList;
}


void FavoritesManager::showWindow() {
    gtk_widget_show_all(window);
}
