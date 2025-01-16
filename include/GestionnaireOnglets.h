#ifndef GESTIONNAIREONGLETS_H
#define GESTIONNAIREONGLETS_H

#include <string>
#include <vector>
#include <unordered_map>
#include <gtk/gtk.h>

struct Onglet {
    std::string url;
    std::string etat; // "actif" ou "hiberné"
};

class GestionnaireOnglets {
public:
    GestionnaireOnglets();

    void ajouterGroupe(const std::string& nomGroupe);
    void supprimerGroupe(const std::string& nomGroupe);
    void ajouterOnglet(const std::string& nomGroupe, const std::string& url);
    void supprimerOnglet(const std::string& nomGroupe, const std::string& url);
    void changerGroupeActif(const std::string& nomGroupe);
    std::vector<std::string> getGroupes() const;
    const std::vector<std::string>& getOngletsDuGroupe(const std::string& nomGroupe) const;
    std::string getGroupeActif() const;

private:
    std::unordered_map<std::string, std::vector<std::string>> groupesOnglets;
    std::string groupeActif;
};

#endif
