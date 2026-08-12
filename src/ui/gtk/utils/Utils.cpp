#include "utils/Utils.h"
#include "managers/FileManager.h"
#include "browser/Browser.h"
#include <iostream>

// Déclarations forward pour les fonctions (définies dans Browser.cpp)
void on_ouvrir_nouvel_onglet_safe(GtkWidget*, gpointer user_data);
void on_modifier_favori(GtkWidget*, gpointer user_data);
void on_supprimer_favori(GtkWidget*, gpointer user_data);

void connecterSignal(GtkWidget* widget, const char* signal, GCallback callback, gpointer data) {
    g_signal_connect(widget, signal, callback, data);
}

static void delete_user_data(gpointer user_data, GClosure*) {
    delete static_cast<std::pair<Browser*, std::string>*>(user_data);
}

void afficherMessageConsole(const std::string& message) {
    std::cout << "[INFO] " << message << std::endl;
}

void chargerFavoris(const std::string& chemin, std::shared_ptr<nlohmann::json>& favorites) {
    nlohmann::json data = FileManager::readJSON(chemin);
    if (!data.is_null() && !data.empty()) {
        *favorites = data;
    }
}

void sauvegarderFavoris(const std::string& chemin, const std::shared_ptr<nlohmann::json>& favorites) {
    FileManager::writeJSON(chemin, *favorites);
}

bool verifierDoublonFavori(const std::shared_ptr<nlohmann::json>& favorites, const std::string& url) {
    return FavoritesJson::containsUrlRecursive(*favorites, url);
}

bool removeFavoriteFromList(std::shared_ptr<nlohmann::json>& favorites, const std::string& favoriteName) {
    return FavoritesJson::removeByNameRecursive(*favorites, favoriteName);
}

void supprimerFavoriGlobal(GtkWidget* widget, gpointer user_data) {
    auto* data = static_cast<std::pair<Browser*, GtkWidget*>*>(user_data);
    if (data && data->first) {
        data->first->removeFavorite(data->second);
    }
    delete data;
}

GtkWidget* creerMenuContextuelFavoris(Browser* navigateur, GtkWidget* widget) {
    GtkWidget* menu = gtk_menu_new();

    GtkWidget* ouvrirNouvelOnglet = gtk_menu_item_new_with_label("Ouvrir dans un nouvel onglet");
    const char* url = static_cast<const char*>(g_object_get_data(G_OBJECT(widget), "favorite-url"));
    std::string urlOuverture = url ? std::string(url) : std::string();
    if (urlOuverture.empty()) {
        urlOuverture = gtk_button_get_label(GTK_BUTTON(widget));
    }
    auto* data = new std::pair<Browser*, std::string>(navigateur, std::move(urlOuverture));
    g_signal_connect_data(ouvrirNouvelOnglet, "activate", G_CALLBACK(on_ouvrir_nouvel_onglet_safe), data, delete_user_data, G_CONNECT_AFTER);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), ouvrirNouvelOnglet);

    GtkWidget* supprimerItem = gtk_menu_item_new_with_label("Supprimer");
    auto* dataSupprimer = new std::pair<Browser*, GtkWidget*>(navigateur, widget);
    g_signal_connect_data(supprimerItem, "activate", G_CALLBACK(on_supprimer_favori), dataSupprimer, delete_user_data, G_CONNECT_AFTER);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), supprimerItem);

    GtkWidget* modifierItem = gtk_menu_item_new_with_label("Modifier");
    auto* dataModifier = new std::pair<Browser*, GtkWidget*>(navigateur, widget);
    g_signal_connect_data(modifierItem, "activate", G_CALLBACK(on_modifier_favori), dataModifier, delete_user_data, G_CONNECT_AFTER);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), modifierItem);

    gtk_widget_show_all(menu);
    return menu;
}
