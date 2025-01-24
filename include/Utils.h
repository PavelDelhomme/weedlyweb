#ifndef UTILS_H
#define UTILS_H

#include <gtk/gtk.h>
#include <nlohmann/json.hpp>
#include <string>
#include <memory>

void connecterSignal(GtkWidget* widget, const char* signal, GCallback callback, gpointer data);
void afficherMessageConsole(const std::string& message);
bool verifierDoublonFavori(const std::shared_ptr<nlohmann::json>& favoris, const std::string& url);
bool supprimerFavoriDeListe(std::shared_ptr<nlohmann::json>& favoris, const std::string& nomFavori);
void supprimerFavoriGlobal(GtkWidget* widget, gpointer user_data);
GtkWidget* creerMenuContextuelFavoris(class Navigateur* navigateur, GtkWidget* widget);
void chargerFavoris(const std::string& chemin, std::shared_ptr<nlohmann::json>& favoris);
void sauvegarderFavoris(const std::string& chemin, const std::shared_ptr<nlohmann::json>& favoris);

#endif // UTILS_H
