#ifndef UTILS_H
#define UTILS_H

#include <gtk/gtk.h>
#include <nlohmann/json.hpp>
#include <string>
#include <memory>

void connecterSignal(GtkWidget* widget, const char* signal, GCallback callback, gpointer data);
void afficherMessageConsole(const std::string& message);
bool verifierDoublonFavori(const std::shared_ptr<nlohmann::json>& favorites, const std::string& url);
bool removeFavoriteFromList(std::shared_ptr<nlohmann::json>& favorites, const std::string& favoriteName);
void supprimerFavoriGlobal(GtkWidget* widget, gpointer user_data);
GtkWidget* creerMenuContextuelFavoris(class Browser* navigateur, GtkWidget* widget);
void chargerFavoris(const std::string& chemin, std::shared_ptr<nlohmann::json>& favorites);
void sauvegarderFavoris(const std::string& chemin, const std::shared_ptr<nlohmann::json>& favorites);

namespace FavoritesJson {
bool isFolder(const nlohmann::json& item);
bool containsUrlRecursive(const nlohmann::json& rootArray, const std::string& url);
bool removeByNameRecursive(nlohmann::json& rootArray, const std::string& name);
bool duplicateNameOrUrl(const nlohmann::json& rootArray, const std::string& nom, const std::string& url);
}

#endif // UTILS_H
