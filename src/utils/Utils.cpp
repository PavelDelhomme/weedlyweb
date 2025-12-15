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

// Fonction pour afficher un message dans la console
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

// Vérification de doublon d'un favori
bool verifierDoublonFavori(const std::shared_ptr<nlohmann::json>& favorites, const std::string& url) {
    return std::any_of(favorites->begin(), favorites->end(), [&](const nlohmann::json& favori) {
        return favori.contains("url") && favori["url"] == url;
    });
}



// Supprimer un favori de la liste
bool removeFavoriteFromList(std::shared_ptr<nlohmann::json>& favorites, const std::string& favoriteName) {
    auto it = std::remove_if(favorites->begin(), favorites->end(), [&](const nlohmann::json& favori) {
        return favori.contains("name") && favori["name"] == favoriteName;
    });
    
    if (it != favorites->end()) {
        favorites->erase(it, favorites->end());
        return true;
    }
    return false;
}


// Fonction globale de suppression d'un favori
void supprimerFavoriGlobal(GtkWidget* widget, gpointer user_data) {
    auto* data = static_cast<std::pair<Browser*, GtkWidget*>*>(user_data);
    if (data && data->first) {
        data->first->removeFavorite(data->second);
    }
    delete data;
}


// Créer un menu contextuel pour les favorites
GtkWidget* creerMenuContextuelFavoris(Browser* navigateur, GtkWidget* widget) {
    GtkWidget* menu = gtk_menu_new();

    // Ouvrir dans un nouvel onglet
    GtkWidget* ouvrirNouvelOnglet = gtk_menu_item_new_with_label("Ouvrir dans un nouvel onglet");
    auto* data = new std::pair<Browser*, std::string>(navigateur, gtk_button_get_label(GTK_BUTTON(widget)));
    g_signal_connect_data(ouvrirNouvelOnglet, "activate", G_CALLBACK(on_ouvrir_nouvel_onglet_safe), data, delete_user_data, G_CONNECT_AFTER);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), ouvrirNouvelOnglet);

    // Supprimer
    GtkWidget* supprimerItem = gtk_menu_item_new_with_label("Supprimer");
    auto* dataSupprimer = new std::pair<Browser*, GtkWidget*>(navigateur, widget);
    g_signal_connect_data(supprimerItem, "activate", G_CALLBACK(on_supprimer_favori), dataSupprimer, delete_user_data, G_CONNECT_AFTER);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), supprimerItem);

    // Modifier le favori
    GtkWidget* modifierItem = gtk_menu_item_new_with_label("Modifier");
    auto* dataModifier = new std::pair<Browser*, GtkWidget*>(navigateur, widget);
    g_signal_connect_data(modifierItem, "activate", G_CALLBACK(on_modifier_favori), dataModifier, delete_user_data, G_CONNECT_AFTER);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), modifierItem);

    gtk_widget_show_all(menu);
    return menu;
}
