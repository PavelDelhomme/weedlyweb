#include "managers/TabsManager.h"
#include <algorithm>
#include <iostream>

TabsManager::TabsManager() {
    ajouterGroupe("Par défaut");
    groupeActif = "Par défaut";
}

void TabsManager::ajouterGroupe(const std::string& groupName) {
    if (groupesOnglets.find(groupName) == groupesOnglets.end()) {
        groupesOnglets[groupName] = {};
    }
}

void TabsManager::supprimerGroupe(const std::string& groupName) {
    if (groupName == "Par défaut") {
        std::cerr << "Impossible de supprimer le groupe par défaut." << std::endl;
        return;
    }
    groupesOnglets.erase(groupName);
}

void TabsManager::ajouterOnglet(const std::string& groupName, const std::string& url) {
    groupesOnglets[groupName].push_back(url);
}

void TabsManager::changerGroupeActif(const std::string& groupName) {
    if (groupesOnglets.find(groupName) != groupesOnglets.end()) {
        groupeActif = groupName;
    }
}

std::vector<std::string> TabsManager::getGroupes() const {
    std::vector<std::string> noms;
    for (const auto& [groupe, _] : groupesOnglets) {
        noms.push_back(groupe);
    }
    return noms;
}

const std::vector<std::string>& TabsManager::getOngletsDuGroupe(const std::string& groupName) const {
    return groupesOnglets.at(groupName);
}

std::string TabsManager::getGroupeActif() const {
    return groupeActif;
}
