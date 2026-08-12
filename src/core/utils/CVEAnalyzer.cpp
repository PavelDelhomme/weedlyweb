#include "utils/CVEAnalyzer.h"
#include <iostream>
#include <regex>
#include <algorithm>
#include <sstream>

std::vector<std::string> CVEAnalyzer::detectCVEs(const std::string& html) {
    std::vector<std::string> cves;
    
    // Expression régulière pour détecter les CVEs (format: CVE-YYYY-NNNNN)
    std::regex cvePattern(R"(CVE-\d{4}-\d{4,7})", std::regex_constants::icase);
    std::sregex_iterator iter(html.begin(), html.end(), cvePattern);
    std::sregex_iterator end;
    
    for (; iter != end; ++iter) {
        std::string cve = iter->str();
        // Convertir en majuscules pour normaliser
        std::transform(cve.begin(), cve.end(), cve.begin(), ::toupper);
        
        // Éviter les doublons
        if (std::find(cves.begin(), cves.end(), cve) == cves.end()) {
            cves.push_back(cve);
        }
    }
    
    return cves;
}

void CVEAnalyzer::displayCVEResults(const std::vector<std::string>& cves) {
    if (cves.empty()) {
        std::cout << "Aucun CVE détecté." << std::endl;
        return;
    }
    
    std::cout << "CVEs détectés (" << cves.size() << ") :" << std::endl;
    for (const auto& cve : cves) {
        std::cout << "  - " << cve << std::endl;
    }
}

std::string CVEAnalyzer::getCVEInfo(const std::string& cveId) {
    // TODO: Implémenter la récupération d'informations depuis une API CVE
    // Par exemple: https://cve.circl.lu/api/cve/{cveId}
    return "Informations CVE non disponibles pour le moment";
}

