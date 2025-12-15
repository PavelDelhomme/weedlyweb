#ifndef TABSMANAGER_H
#define TABSMANAGER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <gtk/gtk.h>

struct Onglet {
    std::string url;
    std::string etat; // "actif" ou "hiberné"
};

class TabsManager {
public:
    TabsManager();

    void ajouterGroupe(const std::string& groupName);
    void supprimerGroupe(const std::string& groupName);
    void ajouterOnglet(const std::string& groupName, const std::string& url);
    void removeTab(const std::string& groupName, const std::string& url);
    void changerGroupeActif(const std::string& groupName);
    std::vector<std::string> getGroupes() const;
    const std::vector<std::string>& getOngletsDuGroupe(const std::string& groupName) const;
    std::string getGroupeActif() const;

private:
    std::unordered_map<std::string, std::vector<std::string>> groupesOnglets;
    std::string groupeActif;
};

#endif
