#include "managers/MemoryManager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <webkit2/webkit2.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <thread>

void MemoryManager::surveillerUtilisationMemoire() {
    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        std::cout << "Mémoire utilisée : " 
                  << (info.totalram - info.freeram) / (1024 * 1024) << " Mo / "
                  << info.totalram / (1024 * 1024) << " Mo" << std::endl;
    } else {
        std::cerr << "Erreur : Impossible de récupérer l'utilisation mémoire." << std::endl;
    }
}

void MemoryManager::optimiserMemoire() {
    WebKitWebContext *context = webkit_web_context_get_default();
    if (context) {
        std::cout << "Optimisation mémoire : vidage du cache WebKit..." << std::endl;
        webkit_web_context_clear_cache(context);
    } else {
        std::cerr << "Erreur : Contexte WebKit non disponible pour optimisation." << std::endl;
    }
}


void MemoryManager::hibernerOnglet(WebKitWebView *onglet) {
    if (onglet && WEBKIT_IS_WEB_VIEW(onglet)) {
        std::cout << "Mise en veille de l'onglet pour économiser des ressources." << std::endl;
        webkit_web_view_stop_loading(onglet); // Arrête les requêtes réseau
        gtk_widget_hide(GTK_WIDGET(onglet));
    }
}

void MemoryManager::reactiverOnglet(WebKitWebView *onglet, const std::string &url) {
    if (onglet && WEBKIT_IS_WEB_VIEW(onglet)) {
        std::cout << "Réactivation de l'onglet avec URL : " << url << std::endl;
        webkit_web_view_load_uri(onglet, url.c_str());
        gtk_widget_show(GTK_WIDGET(onglet));
    }
}

void MemoryManager::viderCache() {
    std::cout << "Vidage du cache WebKit..." << std::endl;

    WebKitWebContext *context = webkit_web_context_get_default();
    if (context) {
        webkit_web_context_clear_cache(context);
        std::cout << "Cache WebKit vidé avec succès." << std::endl;
    } else {
        std::cerr << "Erreur : Impossible de vider le cache. Contexte WebKit non disponible." << std::endl;
    }
}

size_t MemoryManager::obtenirMemoireUtilisee() const {
    // Lire /proc/self/status pour obtenir la mémoire RSS (Resident Set Size)
    std::ifstream statusFile("/proc/self/status");
    std::string line;
    
    while (std::getline(statusFile, line)) {
        if (line.find("VmRSS:") == 0) {
            std::istringstream iss(line);
            std::string label;
            size_t value;
            std::string unit;
            iss >> label >> value >> unit;
            return value; // Retourne en KB
        }
    }
    return 0;
}

size_t MemoryManager::obtenirMemoireVirtuelle() const {
    // Lire /proc/self/status pour obtenir la mémoire virtuelle
    std::ifstream statusFile("/proc/self/status");
    std::string line;
    
    while (std::getline(statusFile, line)) {
        if (line.find("VmSize:") == 0) {
            std::istringstream iss(line);
            std::string label;
            size_t value;
            std::string unit;
            iss >> label >> value >> unit;
            return value; // Retourne en KB
        }
    }
    return 0;
}

void MemoryManager::afficherStatistiquesMemoire() const {
    size_t rss = obtenirMemoireUtilisee();
    size_t vsz = obtenirMemoireVirtuelle();
    
    struct sysinfo info;
    bool sysinfoOk = (sysinfo(&info) == 0);
    
    std::cout << "\n📊 Statistiques de mémoire - WeedlyWeb" << std::endl;
    std::cout << "=====================================" << std::endl;
    std::cout << "💾 Mémoire du processus:" << std::endl;
    std::cout << "  RSS (mémoire physique): " << (rss / 1024.0) << " MB" << std::endl;
    std::cout << "  VSZ (mémoire virtuelle): " << (vsz / 1024.0) << " MB" << std::endl;
    
    if (sysinfoOk) {
        size_t memTotal = info.totalram / (1024 * 1024);
        size_t memFree = info.freeram / (1024 * 1024);
        size_t memUsed = memTotal - memFree;
        double percentUsed = (memUsed * 100.0) / memTotal;
        
        std::cout << "\n🖥️  Mémoire système:" << std::endl;
        std::cout << "  Utilisée: " << memUsed << " MB / " << memTotal << " MB (" 
                  << std::fixed << std::setprecision(1) << percentUsed << "%)" << std::endl;
        std::cout << "  Libre: " << memFree << " MB" << std::endl;
        
        // Pourcentage de mémoire utilisée par WeedlyWeb
        double percentProcess = (rss * 100.0) / (memTotal * 1024);
        std::cout << "\n📈 WeedlyWeb utilise " << std::fixed << std::setprecision(2) 
                  << percentProcess << "% de la mémoire système" << std::endl;
    }
    
    std::cout << "=====================================\n" << std::endl;
}