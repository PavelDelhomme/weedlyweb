#include "MoteurRendu.h"
#include <iostream>

MoteurRendu::MoteurRendu() {
    // Crée une nouvelle vue WebKit
    vueWeb = WEBKIT_WEB_VIEW(webkit_web_view_new());
    definirParametresParDefaut();
}

MoteurRendu::~MoteurRendu() {
    if (vueWeb) {
        g_object_unref(vueWeb);
        vueWeb = nullptr;
    }
}

void MoteurRendu::initialiserRendu(GtkWidget *conteneur) {
    if (!conteneur) {
        std::cerr << "❌ Erreur : Conteneur de rendu non fourni." << std::endl;
        return;
    }
    gtk_box_pack_start(GTK_BOX(conteneur), GTK_WIDGET(vueWeb), TRUE, TRUE, 0);
}

void MoteurRendu::afficherPage(const std::string& url) {
    if (url.empty()) {
        std::cerr << "❌ Erreur : URL vide." << std::endl;
        return;
    }
    std::cout << "🌐 Chargement de l'URL : " << url << std::endl;
    webkit_web_view_load_uri(vueWeb, url.c_str());
}

void MoteurRendu::afficherHTML(const std::string& contenuHTML) {
    if (contenuHTML.empty()) {
        std::cerr << "❌ Erreur : Contenu HTML vide." << std::endl;
        return;
    }
    std::cout << "📜 Affichage du contenu HTML personnalisé." << std::endl;
    webkit_web_view_load_html(vueWeb, contenuHTML.c_str(), nullptr);
}

void MoteurRendu::rafraichirPage() {
    std::cout << "🔄 Rafraîchissement de la page." << std::endl;
    webkit_web_view_reload(vueWeb);
}

void MoteurRendu::arreterChargement() {
    std::cout << "⛔ Arrêt du chargement de la page." << std::endl;
    webkit_web_view_stop_loading(vueWeb);
}

void MoteurRendu::allerEnArriere() {
    std::cout << "⬅️ Navigation en arrière." << std::endl;
    webkit_web_view_go_back(vueWeb);
}

void MoteurRendu::allerEnAvant() {
    std::cout << "➡️ Navigation en avant." << std::endl;
    webkit_web_view_go_forward(vueWeb);
}

std::string MoteurRendu::obtenirURLActuelle() const {
    const gchar *url = webkit_web_view_get_uri(vueWeb);
    if (url) {
        return std::string(url);
    } else {
        return std::string();
    }
}

void MoteurRendu::definirParametresParDefaut() {
    WebKitSettings *settings = webkit_web_view_get_settings(vueWeb);
    g_object_set(G_OBJECT(settings), 
        "enable-javascript", TRUE, 
        "media-playback-requires-user-gesture", TRUE,
        "enable-accelerated-2d-canvas", TRUE,
        NULL);
    std::cout << "⚙️ Paramètres de rendu par défaut définis." << std::endl;
}

void MoteurRendu::activerJavaScript(bool activer) {
    WebKitSettings *settings = webkit_web_view_get_settings(vueWeb);
    g_object_set(G_OBJECT(settings), "enable-javascript", activer, NULL);
    std::cout << "⚙️ JavaScript " << (activer ? "activé" : "désactivé") << "." << std::endl;
}

void MoteurRendu::activerImages(bool activer) {
    WebKitSettings *settings = webkit_web_view_get_settings(vueWeb);
    g_object_set(G_OBJECT(settings), "auto-load-images", activer, NULL);
    std::cout << "🖼️ Chargement des images " << (activer ? "activé" : "désactivé") << "." << std::endl;
}

void MoteurRendu::activerCache(bool activer) {
    WebKitSettings *settings = webkit_web_view_get_settings(vueWeb);
    g_object_set(G_OBJECT(settings), "cache-model", activer ? WEBKIT_CACHE_MODEL_WEB_BROWSER : WEBKIT_CACHE_MODEL_DOCUMENT_VIEWER, NULL);
    std::cout << "📦 Cache " << (activer ? "activé" : "désactivé") << "." << std::endl;
}
