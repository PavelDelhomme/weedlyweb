#include "Utils.h"
#include "Navigateur.h"
#include <iostream>

void connecterSignal(GtkWidget* widget, const char* signal, GCallback callback, gpointer data) {
    g_signal_connect(widget, signal, callback, data);
}

static void delete_user_data(gpointer user_data, GClosure*) {
    delete static_cast<std::pair<Navigateur*, std::string>*>(user_data);
}

// Fonction pour afficher un message dans la console
void afficherMessageConsole(const std::string& message) {
    std::cout << "[INFO] " << message << std::endl;
}

void chargerFavoris(const std::string& chemin, std::shared_ptr<nlohmann::json>& favoris) {
    nlohmann::json data = GestionnaireFichiers::lireJSON(chemin);
    if (!data.is_null() && !data.empty()) {
        *favoris = data;
    }
}
void sauvegarderFavoris(const std::string& chemin, const std::shared_ptr<nlohmann::json>& favoris) {
    GestionnaireFichiers::ecrireJSON(chemin, *favoris);
}

// Vérification de doublon d'un favori
bool verifierDoublonFavori(const std::shared_ptr<nlohmann::json>& favoris, const std::string& url) {
    return std::any_of(favoris->begin(), favoris->end(), [&](const nlohmann::json& favori) {
        return favori.contains("url") && favori["url"] == url;
    });
}



// Supprimer un favori de la liste
bool supprimerFavoriDeListe(std::shared_ptr<nlohmann::json>& favoris, const std::string& nomFavori) {
    auto it = std::remove_if(favoris->begin(), favoris->end(), [&](const nlohmann::json& favori) {
        return favori.contains("name") && favori["name"] == nomFavori;
    });
    
    if (it != favoris->end()) {
        favoris->erase(it, favoris->end());
        return true;
    }
    return false;
}


// Fonction globale de suppression d'un favori
void supprimerFavoriGlobal(GtkWidget* widget, gpointer user_data) {
    auto* data = static_cast<std::pair<Navigateur*, GtkWidget*>*>(user_data);
    if (data && data->first) {
        data->first->supprimerFavori(data->second);
    }
    delete data;
}


// Créer un menu contextuel pour les favoris
GtkWidget* creerMenuContextuelFavoris(Navigateur* navigateur, GtkWidget* widget) {
    GtkWidget* menu = gtk_menu_new();

    // Ouvrir dans un nouvel onglet
    GtkWidget* ouvrirNouvelOnglet = gtk_menu_item_new_with_label("Ouvrir dans un nouvel onglet");
    auto* data = new std::pair<Navigateur*, std::string>(navigateur, gtk_button_get_label(GTK_BUTTON(widget)));
    g_signal_connect_data(ouvrirNouvelOnglet, "activate", G_CALLBACK(on_ouvrir_nouvel_onglet_safe), data, delete_user_data, G_CONNECT_AFTER);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), ouvrirNouvelOnglet);

    // Supprimer
    GtkWidget* supprimerItem = gtk_menu_item_new_with_label("Supprimer");
    auto* dataSupprimer = new std::pair<Navigateur*, GtkWidget*>(navigateur, widget);
    g_signal_connect_data(supprimerItem, "activate", G_CALLBACK(on_supprimer_favori), dataSupprimer, delete_user_data, G_CONNECT_AFTER);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), supprimerItem);

    // Modifier le favori
    GtkWidget* modifierItem = gtk_menu_item_new_with_label("Modifier");
    auto* dataModifier = new std::pair<Navigateur*, GtkWidget*>(navigateur, widget);
    g_signal_connect_data(modifierItem, "activate", G_CALLBACK(on_modifier_favori), dataModifier, delete_user_data, G_CONNECT_AFTER);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), modifierItem);

    gtk_widget_show_all(menu);
    return menu;
}
