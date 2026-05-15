#include "managers/FavoritesManager.h"
#include "utils/Utils.h"
#include "managers/FileManager.h"
#include <iostream>
#include <algorithm>
#include <gtk/gtk.h>

namespace {

static bool modifyFavoriteRecursive(nlohmann::json& arr, const std::string& favoriteName, const std::string& newUrl) {
    for (auto& it : arr) {
        if (FavoritesJson::isFolder(it)) {
            if (modifyFavoriteRecursive(it["children"], favoriteName, newUrl)) {
                return true;
            }
        } else if (it.contains("name") && it["name"].get<std::string>() == favoriteName && it.contains("url")) {
            it["url"] = newUrl;
            return true;
        }
    }
    return false;
}

static void appendFavoritesTreeRows(GtkTreeStore* store, GtkTreeIter* parent, const nlohmann::json& items) {
    if (!items.is_array()) {
        return;
    }
    for (const auto& it : items) {
        if (FavoritesJson::isFolder(it)) {
            GtkTreeIter row;
            gtk_tree_store_append(store, &row, parent);
            const std::string folderLabel = "📁 " + it.value("name", std::string("Dossier"));
            gtk_tree_store_set(store, &row, 0, folderLabel.c_str(), 1, "", -1);
            nlohmann::json children = nlohmann::json::array();
            if (it.contains("children") && it["children"].is_array()) {
                children = it["children"];
            }
            appendFavoritesTreeRows(store, &row, children);
        } else if (it.contains("name") && it.contains("url")) {
            GtkTreeIter row;
            gtk_tree_store_append(store, &row, parent);
            gtk_tree_store_set(store, &row, 0, it["name"].get<std::string>().c_str(), 1, it["url"].get<std::string>().c_str(), -1);
        }
    }
}

static void on_bouton_fermer_clicked(GtkWidget*, gpointer user_data) {
    auto* gestionnaire = static_cast<FavoritesManager*>(user_data);
    if (gestionnaire) {
        gestionnaire->closeWindow();
    }
}

static void on_nouveau_dossier_clicked(GtkWidget*, gpointer user_data) {
    auto* self = static_cast<FavoritesManager*>(user_data);
    if (!self) {
        return;
    }

    GtkWindow* parent = nullptr;
    if (self->getFenetre() && GTK_IS_WINDOW(self->getFenetre())) {
        parent = GTK_WINDOW(self->getFenetre());
    }

    GtkWidget* dlg = gtk_dialog_new_with_buttons(
        "Nouveau dossier",
        parent,
        static_cast<GtkDialogFlags>(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
        "_Annuler",
        GTK_RESPONSE_CANCEL,
        "_Créer",
        GTK_RESPONSE_ACCEPT,
        nullptr);

    GtkWidget* content = gtk_dialog_get_content_area(GTK_DIALOG(dlg));
    GtkWidget* entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Nom du dossier");
    gtk_widget_set_margin_top(entry, 8);
    gtk_widget_set_margin_bottom(entry, 8);
    gtk_widget_set_margin_start(entry, 8);
    gtk_widget_set_margin_end(entry, 8);
    gtk_container_add(GTK_CONTAINER(content), entry);
    gtk_widget_show_all(content);

    const int r = gtk_dialog_run(GTK_DIALOG(dlg));
    if (r == GTK_RESPONSE_ACCEPT) {
        const gchar* t = gtk_entry_get_text(GTK_ENTRY(entry));
        if (t && t[0] != '\0') {
            self->addFolder(t);
        }
    }
    gtk_widget_destroy(dlg);
}

static void on_supprimer_favori(GtkWidget*, gpointer user_data) {
    auto* gestionnaire = static_cast<FavoritesManager*>(user_data);

    GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gestionnaire->getListeFavoris()));
    GtkTreeModel* model = nullptr;
    GtkTreeIter iter;

    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        gchar* favoriteName = nullptr;
        gtk_tree_model_get(model, &iter, 0, &favoriteName, -1);
        if (favoriteName) {
            std::string nameStr(favoriteName);
            g_free(favoriteName);
            const std::string prefix = "📁 ";
            if (nameStr.rfind(prefix, 0) == 0) {
                nameStr = nameStr.substr(prefix.size());
            }
            gestionnaire->removeFavorite(nameStr);
        }
    } else {
        std::cerr << "Aucun favori sélectionné pour suppression." << std::endl;
    }
}

} // namespace

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

void FavoritesManager::createInterface() {
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Gestionnaire de Favoris");
    gtk_window_set_default_size(GTK_WINDOW(window), 640, 420);

    GtkWidget* mainContainer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_add(GTK_CONTAINER(window), mainContainer);

    favoritesList = gtk_tree_view_new();
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(favoritesList), TRUE);
    gtk_box_pack_start(GTK_BOX(mainContainer), favoritesList, TRUE, TRUE, 0);

    GtkWidget* buttonRow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_widget_set_margin_start(buttonRow, 6);
    gtk_widget_set_margin_end(buttonRow, 6);

    GtkWidget* boutonAjouterFavori = gtk_button_new_with_label("Ajouter favori");
    g_signal_connect(boutonAjouterFavori, "clicked", G_CALLBACK(+[](GtkWidget*, gpointer user_data) {
        auto* gestionnaire = static_cast<FavoritesManager*>(user_data);
        if (gestionnaire) {
            gestionnaire->addFavorite("Nouveau favori", "https://example.com", "Général");
        }
    }), this);
    gtk_box_pack_start(GTK_BOX(buttonRow), boutonAjouterFavori, FALSE, FALSE, 0);

    GtkWidget* boutonNouveauDossier = gtk_button_new_with_label("Nouveau dossier");
    g_signal_connect(boutonNouveauDossier, "clicked", G_CALLBACK(on_nouveau_dossier_clicked), this);
    gtk_box_pack_start(GTK_BOX(buttonRow), boutonNouveauDossier, FALSE, FALSE, 0);

    GtkWidget* supprimerItem = gtk_button_new_with_label("Supprimer la sélection");
    g_signal_connect(supprimerItem, "clicked", G_CALLBACK(on_supprimer_favori), this);
    gtk_box_pack_start(GTK_BOX(buttonRow), supprimerItem, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(mainContainer), buttonRow, FALSE, FALSE, 0);

    GtkWidget* boutonFermer = gtk_button_new_with_label("Fermer");
    g_signal_connect(boutonFermer, "clicked", G_CALLBACK(on_bouton_fermer_clicked), this);
    gtk_box_pack_start(GTK_BOX(mainContainer), boutonFermer, FALSE, FALSE, 0);

    displayFavoritesList();
}

void FavoritesManager::displayFavoritesList() {
    GtkTreeStore* store = gtk_tree_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
    appendFavoritesTreeRows(store, nullptr, *favorites);

    GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn* colNom = gtk_tree_view_column_new_with_attributes("Nom", renderer, "text", 0, nullptr);
    GtkTreeViewColumn* colURL = gtk_tree_view_column_new_with_attributes("URL", renderer, "text", 1, nullptr);
    gtk_tree_view_column_set_expand(colURL, TRUE);

    gtk_tree_view_append_column(GTK_TREE_VIEW(favoritesList), colNom);
    gtk_tree_view_append_column(GTK_TREE_VIEW(favoritesList), colURL);
    gtk_tree_view_set_model(GTK_TREE_VIEW(favoritesList), GTK_TREE_MODEL(store));
    g_object_unref(store);
}

void FavoritesManager::addFolder(const std::string& nom) {
    if (nom.empty()) {
        return;
    }
    if (FavoritesJson::duplicateNameOrUrl(*favorites, nom, "")) {
        std::cerr << "Un élément porte déjà ce nom : " << nom << std::endl;
        return;
    }
    favorites->push_back(nlohmann::json::object(
        {{"name", nom}, {"type", "folder"}, {"children", nlohmann::json::array()}}));
    saveModifications();
    callbackRafraichir();
    refreshInterface();
}

void FavoritesManager::addFavorite(const std::string& nom, const std::string& url, const std::string& tag) {
    if (FavoritesJson::duplicateNameOrUrl(*favorites, nom, url)) {
        std::cerr << "Le favori existe déjà (nom ou URL) : " << nom << " (" << url << ")" << std::endl;
        return;
    }

    favorites->push_back(nlohmann::json::object({{"name", nom}, {"url", url}, {"tag", tag}}));

    saveModifications();
    callbackRafraichir();
    refreshInterface();
}

void FavoritesManager::removeFavorite(const std::string& favoriteName) {
    if (FavoritesJson::removeByNameRecursive(*favorites, favoriteName)) {
        saveModifications();
        afficherMessageConsole("Favori supprimé avec succès : " + favoriteName);
        callbackRafraichir();
        refreshInterface();
    } else {
        afficherMessageConsole("Erreur : Impossible de trouver le favori : " + favoriteName);
    }
}

bool FavoritesManager::removeFavoriteFromList(const std::string& favoriteName) {
    return FavoritesJson::removeByNameRecursive(*favorites, favoriteName);
}

void FavoritesManager::modifyFavorite(const std::string& favoriteName, const std::string& newUrl) {
    if (modifyFavoriteRecursive(*favorites, favoriteName, newUrl)) {
        saveModifications();
        afficherMessageConsole("Favori modifié : " + favoriteName + " -> " + newUrl);
        callbackRafraichir();
        refreshInterface();
    } else {
        afficherMessageConsole("Erreur : Favori non trouvé pour modification : " + favoriteName);
    }
}

void FavoritesManager::createFavoriteContextMenu(GtkWidget* button, const std::string& favoriteName) {
    GtkWidget* menu = gtk_menu_new();

    GtkWidget* ouvrirNouvelOngletItem = gtk_menu_item_new_with_label("Ouvrir dans un nouvel onglet");
    g_signal_connect(ouvrirNouvelOngletItem, "activate", G_CALLBACK(+[](GtkWidget*, gpointer user_data) {
        auto* gestionnaire = static_cast<FavoritesManager*>(user_data);
        gestionnaire->callbackRafraichir();
    }), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), ouvrirNouvelOngletItem);

    GtkWidget* modifierItem = gtk_menu_item_new_with_label("Modifier l’URL");
    gtk_widget_set_sensitive(modifierItem, FALSE);
    gtk_widget_set_tooltip_text(modifierItem, "À venir : édition depuis cette fenêtre. Modifiez favorites.json ou utilisez le menu du navigateur.");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), modifierItem);

    GtkWidget* supprimerItem = gtk_menu_item_new_with_label("Supprimer");
    g_object_set_data_full(G_OBJECT(supprimerItem), "favoriteName", g_strdup(favoriteName.c_str()), g_free);
    g_signal_connect(supprimerItem, "activate", G_CALLBACK(+[](GtkWidget* widget, gpointer user_data) {
        auto* gestionnaire = static_cast<FavoritesManager*>(user_data);
        const char* name = static_cast<const char*>(g_object_get_data(G_OBJECT(widget), "favoriteName"));
        gestionnaire->removeFavorite(name ? name : "");
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
        if (!gtk_widget_in_destruction(favoritesList)) {
            GtkWidget* parent = gtk_widget_get_parent(favoritesList);
            if (parent && GTK_IS_CONTAINER(parent)) {
                gtk_container_remove(GTK_CONTAINER(parent), favoritesList);
            }
            gtk_widget_destroy(favoritesList);
        }
        favoritesList = nullptr;
    }
    favoritesList = gtk_tree_view_new();
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(favoritesList), TRUE);
    displayFavoritesList();

    if (window && GTK_IS_WIDGET(window)) {
        GtkWidget* mainContainer = gtk_bin_get_child(GTK_BIN(window));
        if (mainContainer && GTK_IS_BOX(mainContainer)) {
            gtk_box_pack_start(GTK_BOX(mainContainer), favoritesList, TRUE, TRUE, 0);
            gtk_box_reorder_child(GTK_BOX(mainContainer), favoritesList, 0);
        }
        gtk_widget_show_all(window);
    }
}

GtkWidget* FavoritesManager::getListeFavoris() {
    return favoritesList;
}

void FavoritesManager::showWindow() {
    gtk_widget_show_all(window);
}
