#include "GestionnaireOnglets.h"
#include <algorithm>
#include <iostream>

GestionnaireOnglets::GestionnaireOnglets() {
    ajouterGroupe("Par défaut");
    groupeActif = "Par défaut";
}

void GestionnaireOnglets::ajouterGroupe(const std::string& nomGroupe) {
    if (groupesOnglets.find(nomGroupe) == groupesOnglets.end()) {
        groupesOnglets[nomGroupe] = {};
    }
}

void GestionnaireOnglets::supprimerGroupe(const std::string& nomGroupe) {
    if (nomGroupe == "Par défaut") {
        std::cerr << "Impossible de supprimer le groupe par défaut." << std::endl;
        return;
    }
    groupesOnglets.erase(nomGroupe);
}

void GestionnaireOnglets::ajouterOnglet(const std::string& nomGroupe, const std::string& url) {
    groupesOnglets[nomGroupe].push_back(url);
}

void GestionnaireOnglets::changerGroupeActif(const std::string& nomGroupe) {
    if (groupesOnglets.find(nomGroupe) != groupesOnglets.end()) {
        groupeActif = nomGroupe;
    }
}

std::vector<std::string> GestionnaireOnglets::getGroupes() const {
    std::vector<std::string> noms;
    for (const auto& [groupe, _] : groupesOnglets) {
        noms.push_back(groupe);
    }
    return noms;
}

const std::vector<std::string>& GestionnaireOnglets::getOngletsDuGroupe(const std::string& nomGroupe) const {
    return groupesOnglets.at(nomGroupe);
}

std::string GestionnaireOnglets::getGroupeActif() const {
    return groupeActif;
}
