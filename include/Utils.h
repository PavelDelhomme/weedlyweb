#ifndef UTILS_H
#define UTILS_H

#include <gtk/gtk.h>
#include <nlohmann/json.hpp>
#include <string>
#include <memory>

void afficherMessageConsole(const std::string& message);
bool verifierDoublonFavori(const std::shared_ptr<nlohmann::json>& favoris, const std::string& url);
bool supprimerFavoriDeListe(std::shared_ptr<nlohmann::json>& favoris, const std::string& nomFavori);
void supprimerFavoriGlobal(GtkWidget* widget, gpointer user_data);
GtkWidget* creerMenuContextuelFavoris(class Navigateur* navigateur, GtkWidget* widget);

#endif // UTILS_H
