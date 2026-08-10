#include "utils/Utils.h"
#include "managers/FileManager.h"
#include "browser/Browser.h"
#include <cctype>
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

namespace FavoritesJson {

bool isFolder(const nlohmann::json& item) {
    return item.is_object() && item.contains("type") && item["type"] == "folder" && item.contains("children") &&
           item["children"].is_array();
}

std::string canonicalizeUrl(const std::string& url) {
    std::string u = url;
    // minuscules
    for (char& c : u) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    // enlever fragment
    const auto hash = u.find('#');
    if (hash != std::string::npos) {
        u.erase(hash);
    }
    // enlever slash final (sauf schéma seul)
    while (u.size() > 8 && u.back() == '/') {
        u.pop_back();
    }
    // unifier http/https host www.
    auto stripWww = [](std::string s) {
        const std::string markers[] = {"https://www.", "http://www.", "https://", "http://"};
        for (const auto& m : markers) {
            if (s.rfind(m, 0) == 0) {
                s = s.substr(m.size());
                break;
            }
        }
        return s;
    };
    return stripWww(u);
}

bool urlsMatch(const std::string& a, const std::string& b) {
    if (a == b) {
        return true;
    }
    return canonicalizeUrl(a) == canonicalizeUrl(b);
}

bool containsUrlRecursive(const nlohmann::json& rootArray, const std::string& url) {
    if (!rootArray.is_array()) {
        return false;
    }
    for (const auto& item : rootArray) {
        if (isFolder(item)) {
            if (containsUrlRecursive(item["children"], url)) {
                return true;
            }
        } else if (item.contains("url") && item["url"].is_string() &&
                   urlsMatch(item["url"].get<std::string>(), url)) {
            return true;
        }
    }
    return false;
}

bool removeByNameRecursive(nlohmann::json& rootArray, const std::string& name) {
    if (!rootArray.is_array()) {
        return false;
    }
    for (auto it = rootArray.begin(); it != rootArray.end(); ++it) {
        if (!it->is_object() || !(*it).contains("name")) {
            continue;
        }
        if ((*it)["name"].get<std::string>() == name) {
            rootArray.erase(it);
            return true;
        }
        if (isFolder(*it)) {
            if (removeByNameRecursive((*it)["children"], name)) {
                return true;
            }
        }
    }
    return false;
}

bool removeByUrlRecursive(nlohmann::json& rootArray, const std::string& url) {
    if (!rootArray.is_array()) {
        return false;
    }
    for (auto it = rootArray.begin(); it != rootArray.end(); ++it) {
        if (!it->is_object()) {
            continue;
        }
        if (isFolder(*it)) {
            if (removeByUrlRecursive((*it)["children"], url)) {
                return true;
            }
        } else if ((*it).contains("url") && (*it)["url"].is_string() &&
                   urlsMatch((*it)["url"].get<std::string>(), url)) {
            rootArray.erase(it);
            return true;
        }
    }
    return false;
}

bool duplicateNameOrUrl(const nlohmann::json& rootArray, const std::string& nom, const std::string& url) {
    if (!rootArray.is_array()) {
        return false;
    }
    for (const auto& item : rootArray) {
        if (isFolder(item)) {
            if (item.contains("name") && item["name"].is_string() && item["name"].get<std::string>() == nom) {
                return true;
            }
            if (duplicateNameOrUrl(item["children"], nom, url)) {
                return true;
            }
        } else {
            if (item.contains("name") && item["name"].is_string() && item["name"].get<std::string>() == nom) {
                return true;
            }
            if (item.contains("url") && item["url"].is_string() &&
                urlsMatch(item["url"].get<std::string>(), url)) {
                return true;
            }
        }
    }
    return false;
}

} // namespace FavoritesJson

// Vérification de doublon d'un favori (y compris dans les dossiers)
bool verifierDoublonFavori(const std::shared_ptr<nlohmann::json>& favorites, const std::string& url) {
    return FavoritesJson::containsUrlRecursive(*favorites, url);
}



// Supprimer un favori de la liste (y compris dans les dossiers)
bool removeFavoriteFromList(std::shared_ptr<nlohmann::json>& favorites, const std::string& favoriteName) {
    return FavoritesJson::removeByNameRecursive(*favorites, favoriteName);
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
    const char* url = static_cast<const char*>(g_object_get_data(G_OBJECT(widget), "favorite-url"));
    std::string urlOuverture = url ? std::string(url) : std::string();
    if (urlOuverture.empty()) {
        urlOuverture = gtk_button_get_label(GTK_BUTTON(widget));
    }
    auto* data = new std::pair<Browser*, std::string>(navigateur, std::move(urlOuverture));
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
