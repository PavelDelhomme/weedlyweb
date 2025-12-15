#ifndef CVEANALYZER_H
#define CVEANALYZER_H

#include <string>
#include <vector>
#include <regex>

class CVEAnalyzer {
public:
    // Détecter les CVEs dans le contenu HTML
    static std::vector<std::string> detectCVEs(const std::string& html);
    
    // Afficher les résultats des CVEs détectés
    static void displayCVEResults(const std::vector<std::string>& cves);
    
    // Obtenir des informations sur un CVE (via API si disponible)
    static std::string getCVEInfo(const std::string& cveId);
};

#endif // CVEANALYZER_H

