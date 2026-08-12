#ifndef UTILS_H
#define UTILS_H

#include <gtk/gtk.h>
#include <nlohmann/json.hpp>
#include <string>
#include <memory>
#include "core/FavoritesJson.h"

void connecterSignal(GtkWidget* widget, const char* signal, GCallback callback, gpointer data);
void afficherMessageConsole(const std::string& message);
bool verifierDoublonFavori(const std::shared_ptr<nlohmann::json>& favorites, const std::string& url);
bool removeFavoriteFromList(std::shared_ptr<nlohmann::json>& favorites, const std::string& favoriteName);
void supprimerFavoriGlobal(GtkWidget* widget, gpointer user_data);
GtkWidget* creerMenuContextuelFavoris(class Browser* navigateur, GtkWidget* widget);
void chargerFavoris(const std::string& chemin, std::shared_ptr<nlohmann::json>& favorites);
void sauvegarderFavoris(const std::string& chemin, const std::shared_ptr<nlohmann::json>& favorites);

#endif // UTILS_H
