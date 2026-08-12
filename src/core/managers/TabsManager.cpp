#include "managers/TabsManager.h"
#include <algorithm>
#include <iostream>

const std::vector<std::string> TabsManager::kPalette = {
    "#FB542B", // orange Brave
    "#A0A5EB", // lavande
    "#4C54D2", // bleu
    "#F39026", // ambre
    "#E22172", // rose
    "#0076FF", // cyan
    "#079D70", // vert
    "#9E1F63", // magenta
};

std::string TabsManager::prochaineCouleur(size_t index) {
    if (kPalette.empty()) {
        return "#4C54D2";
    }
    return kPalette[index % kPalette.size()];
}

TabsManager::TabsManager() {
    ajouterGroupe("Par défaut", "#4C54D2");
    // Groupes de test (style Brave) pour basculer / ouvrir / supprimer
    ajouterGroupe("Travail", "#FB542B");
    ajouterGroupe("Perso", "#079D70");
    ajouterGroupe("Dev & Cyber", "#0076FF");
    ajouterGroupe("Recherche", "#F39026");
    groupeActif = "Par défaut";
}

void TabsManager::ajouterGroupe(const std::string& groupName, const std::string& color) {
    if (groupesOnglets.find(groupName) == groupesOnglets.end()) {
        groupesOnglets[groupName] = {};
        if (!color.empty()) {
            couleursGroupes[groupName] = color;
        } else {
            couleursGroupes[groupName] = prochaineCouleur(groupesOnglets.size() - 1);
        }
    }
}

void TabsManager::supprimerGroupe(const std::string& groupName) {
    if (groupName == "Par défaut") {
        std::cerr << "Impossible de supprimer le groupe par défaut." << std::endl;
        return;
    }
    groupesOnglets.erase(groupName);
    couleursGroupes.erase(groupName);
    if (groupeActif == groupName) {
        groupeActif = "Par défaut";
    }
}

void TabsManager::renommerGroupe(const std::string& oldName, const std::string& newName) {
    if (oldName == "Par défaut" || newName.empty() || oldName == newName) {
        return;
    }
    if (groupesOnglets.find(oldName) == groupesOnglets.end()) {
        return;
    }
    if (groupesOnglets.find(newName) != groupesOnglets.end()) {
        return;
    }
    groupesOnglets[newName] = std::move(groupesOnglets[oldName]);
    groupesOnglets.erase(oldName);
    couleursGroupes[newName] = couleursGroupes.count(oldName) ? couleursGroupes[oldName] : prochaineCouleur(0);
    couleursGroupes.erase(oldName);
    if (groupeActif == oldName) {
        groupeActif = newName;
    }
}

void TabsManager::setCouleurGroupe(const std::string& groupName, const std::string& color) {
    if (groupesOnglets.find(groupName) != groupesOnglets.end()) {
        couleursGroupes[groupName] = color;
    }
}

std::string TabsManager::getCouleurGroupe(const std::string& groupName) const {
    auto it = couleursGroupes.find(groupName);
    if (it != couleursGroupes.end()) {
        return it->second;
    }
    return "#4C54D2";
}

void TabsManager::ajouterOnglet(const std::string& groupName, const std::string& url) {
    if (groupesOnglets.find(groupName) == groupesOnglets.end()) {
        ajouterGroupe(groupName);
    }
    groupesOnglets[groupName].push_back(url);
}

void TabsManager::removeTab(const std::string& groupName, const std::string& url) {
    auto it = groupesOnglets.find(groupName);
    if (it == groupesOnglets.end()) {
        return;
    }
    auto& vec = it->second;
    auto found = std::find(vec.begin(), vec.end(), url);
    if (found != vec.end()) {
        vec.erase(found);
    }
}

void TabsManager::changerGroupeActif(const std::string& groupName) {
    if (groupesOnglets.find(groupName) != groupesOnglets.end()) {
        groupeActif = groupName;
    }
}

std::vector<std::string> TabsManager::getGroupes() const {
    std::vector<std::string> noms;
    // Par défaut en premier
    if (groupesOnglets.count("Par défaut")) {
        noms.push_back("Par défaut");
    }
    for (const auto& [groupe, _] : groupesOnglets) {
        if (groupe != "Par défaut") {
            noms.push_back(groupe);
        }
    }
    return noms;
}

const std::vector<std::string>& TabsManager::getOngletsDuGroupe(const std::string& groupName) const {
    return groupesOnglets.at(groupName);
}

std::string TabsManager::getGroupeActif() const {
    return groupeActif;
}

bool TabsManager::groupeExiste(const std::string& groupName) const {
    return groupesOnglets.find(groupName) != groupesOnglets.end();
}
