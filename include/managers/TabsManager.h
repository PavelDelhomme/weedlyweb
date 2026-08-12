#ifndef TABSMANAGER_H
#define TABSMANAGER_H

#include <string>
#include <vector>
#include <unordered_map>

class TabsManager {
public:
    TabsManager();

    void ajouterGroupe(const std::string& groupName, const std::string& color = "");
    void supprimerGroupe(const std::string& groupName);
    void renommerGroupe(const std::string& oldName, const std::string& newName);
    void setCouleurGroupe(const std::string& groupName, const std::string& color);
    std::string getCouleurGroupe(const std::string& groupName) const;

    void ajouterOnglet(const std::string& groupName, const std::string& url);
    void removeTab(const std::string& groupName, const std::string& url);
    void changerGroupeActif(const std::string& groupName);

    std::vector<std::string> getGroupes() const;
    const std::vector<std::string>& getOngletsDuGroupe(const std::string& groupName) const;
    std::string getGroupeActif() const;
    bool groupeExiste(const std::string& groupName) const;

    /** Palette Brave-like pour nouveaux groupes */
    static std::string prochaineCouleur(size_t index);

private:
    std::unordered_map<std::string, std::vector<std::string>> groupesOnglets;
    std::unordered_map<std::string, std::string> couleursGroupes;
    std::string groupeActif;
    static const std::vector<std::string> kPalette;
};

#endif
