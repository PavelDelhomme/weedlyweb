#include "MoteurRendu.h"
#include <iostream>
#include <memory>

// Fonction statique pour gérer "notify::title"
static void on_notify_title(GObject *object, GParamSpec *param_spec, gpointer user_data) {
    auto *data = static_cast<std::pair<WebKitWebView*, std::function<void(const std::string&)>*>*>(user_data);
    WebKitWebView* vueWeb = data->first;

    if (!WEBKIT_IS_WEB_VIEW(vueWeb)) {
        std::cerr << "Erreur : WebView invalide dans on_notify_title." << std::endl;
        return;
    }

    const gchar* title = webkit_web_view_get_title(vueWeb);
    if (title) {
        auto& callback = data->second;
        (*callback)(std::string(title));
    }
}

// Fonction statique pour gérer "notify::uri"
static void on_notify_uri(GObject *object, GParamSpec *param_spec, gpointer user_data) {
    auto* data = static_cast<std::pair<WebKitWebView*, std::function<void(const std::string&)>>*>(user_data);
    if (!data || !WEBKIT_IS_WEB_VIEW(data->first)) {
        std::cerr << "Erreur : WebView invalide dans on_notify_uri." << std::endl;
        return;
    }

    WebKitWebView* vueWeb = data->first;
    const gchar* uri = webkit_web_view_get_uri(vueWeb);
    if (uri) {
        data->second(std::string(uri));
    }
}

// Fonction statique pour gérer "destroy"
static void on_destroy_callback(GObject *object, gpointer user_data) {
    auto *data = static_cast<std::pair<WebKitWebView*, std::function<void(const std::string&)>*>*>(user_data);
    delete data; // Libère la mémoire allouée dynamiquement
}


MoteurRendu::MoteurRendu() {
    vueWeb = WEBKIT_WEB_VIEW(webkit_web_view_new());
    if (!vueWeb) {
        std::cerr << "Erreur : Impossible de créer une instance de WebKitWebView." << std::endl;
        throw std::runtime_error("WebKitWebView non initialisé.");
    }
}

MoteurRendu::~MoteurRendu() {
    if (vueWeb) {
        g_clear_object(&vueWeb);
    }
}


void MoteurRendu::initialiserRendu(GtkWidget *conteneurPrincipal) {
    GtkWidget *conteneur = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(conteneur), GTK_WIDGET(vueWeb), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(conteneurPrincipal), conteneur, TRUE, TRUE, 0);
}

void MoteurRendu::afficherPage(const std::string& url) {
    if (!vueWeb) {
        std::cerr << "Erreur : WebView non initialisé dans afficherPage." << std::endl;
        return;
    }
    std::cout << "Chargement de l'URL : " << url << std::endl;
    try {
        webkit_web_view_load_uri(vueWeb, url.c_str());
    } catch (const std::exception& e) {
        std::cerr << "Exception lors du chargement de l'URL : " << e.what() << std::endl;
    }
}


std::string MoteurRendu::obtenirURLActuelle() const {
    const gchar *url = webkit_web_view_get_uri(vueWeb);
    return url ? std::string(url) : "";
}


void MoteurRendu::naviguerRetour() {
    if (webkit_web_view_can_go_back(vueWeb)) {
        webkit_web_view_go_back(vueWeb);
    }
}

void MoteurRendu::naviguerSuivant() {
    if (webkit_web_view_can_go_forward(vueWeb)) {
        webkit_web_view_go_forward(vueWeb);
    }
}

GtkWidget* MoteurRendu::creerBouton(const std::string& iconName, GCallback callback, gpointer data) {
    GtkWidget *image = gtk_image_new_from_icon_name(iconName.c_str(), GTK_ICON_SIZE_BUTTON);
    GtkWidget *bouton = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(bouton), image);
    g_signal_connect(bouton, "clicked", callback, data);
    return bouton;
}


GtkWidget* MoteurRendu::creerChampTexte(GCallback callback, gpointer data) {
    GtkWidget *champ = gtk_entry_new();
    g_signal_connect(champ, "activate", callback, data);
    return champ;
}

void MoteurRendu::connecterSignalPageChargee(std::function<void(const std::string&)> callback) {
    if (!vueWeb || !callback) return;
    if (!vueWeb || !WEBKIT_IS_WEB_VIEW(vueWeb)) {
        std::cerr << "Erreur : WebView invalide dans connecterSignal." << std::endl;
        return;
    }


    auto data = new std::pair<WebKitWebView*, std::function<void(const std::string&)>>(vueWeb, callback);

    g_signal_connect_data(
        vueWeb,
        "notify::title",
        G_CALLBACK(on_notify_title),
        data,
        [](gpointer user_data, GClosure*) {
            delete static_cast<std::pair<WebKitWebView*, std::function<void(const std::string&)>>*>(user_data);
        },
        G_CONNECT_SWAPPED
    );
}


void MoteurRendu::rafraichirPage() {
    if (vueWeb && WEBKIT_IS_WEB_VIEW(vueWeb)) {
        webkit_web_view_reload(vueWeb);
    }
}

void MoteurRendu::connecterSignalURLChangee(std::function<void(const std::string&)> callback) {
    if (!vueWeb || !WEBKIT_IS_WEB_VIEW(vueWeb)) {
        std::cerr << "Erreur : WebView invalide dans connecterSignalURLChangee." << std::endl;
        return;
    }

    auto data = new std::pair<WebKitWebView*, std::function<void(const std::string&)>>(vueWeb, callback);

    g_signal_connect_data(
        vueWeb,
        "notify::uri",
        G_CALLBACK(on_notify_uri),
        data,
        [](gpointer user_data, GClosure*) {
            delete static_cast<std::pair<WebKitWebView*, std::function<void(const std::string&)>>*>(user_data);
        },
        G_CONNECT_SWAPPED
    );
}


void MoteurRendu::onNotifyUri(GObject *object, GParamSpec *param_spec, gpointer user_data) {
    auto* data = static_cast<std::pair<WebKitWebView*, std::function<void(const std::string&)>>*>(user_data);
    if (!data || !WEBKIT_IS_WEB_VIEW(data->first)) return;

    const gchar* uri = webkit_web_view_get_uri(data->first);
    if (uri) data->second(std::string(uri));
}
